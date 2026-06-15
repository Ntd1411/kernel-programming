/*
 * select_server.c - Server sử dụng select() để xử lý nhiều kết nối
 * 
 * select() cho phép monitor nhiều file descriptors đồng thời
 * 
 * Biên dịch: gcc -o select_server select_server.c
 * Chạy: ./select_server <port>
 * Test: telnet localhost <port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024

void handle_client_message(int client_fd, char *buffer, ssize_t bytes_read) {
    printf("[Client %d] Received %zd bytes: %.*s", 
           client_fd, bytes_read, (int)bytes_read, buffer);
    
    // Echo back
    if (send(client_fd, buffer, bytes_read, 0) != bytes_read) {
        perror("send");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Sử dụng: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int port = atoi(argv[1]);
    int server_fd, max_fd, client_fds[MAX_CLIENTS];
    fd_set read_fds, master_fds;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    
    // Khởi tạo mảng client fds
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_fds[i] = -1;
    }
    
    // Tạo socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Cho phép reuse address
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
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
    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Select Server đang lắng nghe trên port %d\n", port);
    printf("Số lượng clients tối đa: %d\n", MAX_CLIENTS);
    printf("Nhấn Ctrl+C để dừng\n\n");
    
    // Khởi tạo master fd set
    FD_ZERO(&master_fds);
    FD_SET(server_fd, &master_fds);
    max_fd = server_fd;
    
    while (1) {
        // Copy master set vào read set
        read_fds = master_fds;
        
        // Select
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }
        
        // Kiểm tra server socket - có kết nối mới
        if (FD_ISSET(server_fd, &read_fds)) {
            int new_client = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
            if (new_client < 0) {
                perror("accept");
                continue;
            }
            
            printf("Kết nối mới từ %s:%d (fd=%d)\n",
                   inet_ntoa(client_addr.sin_addr),
                   ntohs(client_addr.sin_port),
                   new_client);
            
            // Thêm vào danh sách clients
            int added = 0;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_fds[i] == -1) {
                    client_fds[i] = new_client;
                    FD_SET(new_client, &master_fds);
                    if (new_client > max_fd) {
                        max_fd = new_client;
                    }
                    added = 1;
                    printf("Client được thêm vào slot %d\n", i);
                    break;
                }
            }
            
            if (!added) {
                printf("Đã đạt số lượng clients tối đa. Từ chối kết nối.\n");
                close(new_client);
            }
            
            // Gửi welcome message
            const char *welcome = "Chào mừng đến server!\n";
            send(new_client, welcome, strlen(welcome), 0);
        }
        
        // Kiểm tra các client sockets
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int client_fd = client_fds[i];
            
            if (client_fd == -1) continue;
            
            if (FD_ISSET(client_fd, &read_fds)) {
                ssize_t bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
                
                if (bytes_read <= 0) {
                    // Client đã ngắt kết nối
                    if (bytes_read == 0) {
                        printf("Client %d (fd=%d) đã ngắt kết nối\n", i, client_fd);
                    } else {
                        perror("recv");
                    }
                    
                    close(client_fd);
                    FD_CLR(client_fd, &master_fds);
                    client_fds[i] = -1;
                } else {
                    // Xử lý dữ liệu
                    buffer[bytes_read] = '\0';
                    handle_client_message(client_fd, buffer, bytes_read);
                }
            }
        }
    }
    
    // Cleanup
    close(server_fd);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_fds[i] != -1) {
            close(client_fds[i]);
        }
    }
    
    return 0;
}
