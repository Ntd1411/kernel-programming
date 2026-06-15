/*
 * poll_server.c - Server sử dụng poll() để xử lý nhiều kết nối
 * 
 * poll() hiệu quả hơn select() với số lượng lớn file descriptors
 * 
 * Biên dịch: gcc -o poll_server poll_server.c
 * Chạy: ./poll_server <port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 200
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Sử dụng: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int port = atoi(argv[1]);
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    struct pollfd fds[MAX_CLIENTS + 1];
    int nfds = 1;
    char buffer[BUFFER_SIZE];
    
    // Tạo socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Listen
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Poll Server đang lắng nghe trên port %d\n", port);
    printf("Số lượng clients tối đa: %d\n", MAX_CLIENTS);
    printf("Nhấn Ctrl+C để dừng\n\n");
    
    // Khởi tạo pollfd array
    memset(fds, 0, sizeof(fds));
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;
    
    while (1) {
        int ret = poll(fds, nfds, -1);
        
        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }
        
        // Kiểm tra server socket
        if (fds[0].revents & POLLIN) {
            int new_client = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
            if (new_client < 0) {
                perror("accept");
                continue;
            }
            
            printf("Kết nối mới từ %s:%d (fd=%d)\n",
                   inet_ntoa(client_addr.sin_addr),
                   ntohs(client_addr.sin_port),
                   new_client);
            
            if (nfds < MAX_CLIENTS + 1) {
                fds[nfds].fd = new_client;
                fds[nfds].events = POLLIN;
                nfds++;
                printf("Client được thêm. Tổng số clients: %d\n", nfds - 1);
                
                const char *welcome = "Chào mừng đến Poll Server!\n";
                send(new_client, welcome, strlen(welcome), 0);
            } else {
                printf("Đã đạt số lượng clients tối đa. Từ chối kết nối.\n");
                close(new_client);
            }
        }
        
        // Kiểm tra các client sockets
        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                ssize_t bytes_read = recv(fds[i].fd, buffer, BUFFER_SIZE - 1, 0);
                
                if (bytes_read <= 0) {
                    if (bytes_read == 0) {
                        printf("Client (fd=%d) đã ngắt kết nối\n", fds[i].fd);
                    } else {
                        perror("recv");
                    }
                    
                    close(fds[i].fd);
                    
                    // Di chuyển phần tử cuối vào vị trí này
                    fds[i] = fds[nfds - 1];
                    nfds--;
                    i--;
                    
                    printf("Client removed. Còn lại: %d clients\n", nfds - 1);
                } else {
                    buffer[bytes_read] = '\0';
                    printf("[Client fd=%d] %.*s", fds[i].fd, (int)bytes_read, buffer);
                    
                    // Echo back
                    send(fds[i].fd, buffer, bytes_read, 0);
                }
            }
            
            if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                printf("Client (fd=%d) lỗi hoặc ngắt kết nối\n", fds[i].fd);
                close(fds[i].fd);
                fds[i] = fds[nfds - 1];
                nfds--;
                i--;
            }
        }
    }
    
    // Cleanup
    close(server_fd);
    for (int i = 1; i < nfds; i++) {
        close(fds[i].fd);
    }
    
    return 0;
}
