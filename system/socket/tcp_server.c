/*
 * tcp_server.c - TCP Server đơn giản
 * 
 * Server lắng nghe kết nối TCP và xử lý từng client tuần tự
 * 
 * Biên dịch: gcc -o tcp_server tcp_server.c
 * Chạy: ./tcp_server <port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define BACKLOG 5

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Sử dụng: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int port = atoi(argv[1]);
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    
    // Tạo socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    printf("Socket được tạo thành công (fd=%d)\n", server_fd);
    
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
    
    printf("Socket được bind vào port %d\n", port);
    
    // Listen
    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Server đang lắng nghe trên port %d...\n", port);
    printf("Nhấn Ctrl+C để dừng\n\n");
    
    // Accept loop
    while (1) {
        printf("Đang chờ kết nối...\n");
        
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }
        
        printf("\n=== Kết nối mới ===\n");
        printf("Client: %s:%d\n", 
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));
        printf("Socket fd: %d\n\n", client_fd);
        
        // Gửi welcome message
        const char *welcome = "Chào mừng đến TCP Server!\n";
        send(client_fd, welcome, strlen(welcome), 0);
        
        // Nhận và xử lý dữ liệu
        while (1) {
            ssize_t bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
            
            if (bytes_read < 0) {
                perror("recv");
                break;
            } else if (bytes_read == 0) {
                printf("Client đã ngắt kết nối\n\n");
                break;
            }
            
            buffer[bytes_read] = '\0';
            printf("Nhận được (%zd bytes): %s", bytes_read, buffer);
            
            // Echo back
            if (send(client_fd, buffer, bytes_read, 0) < 0) {
                perror("send");
                break;
            }
        }
        
        close(client_fd);
    }
    
    close(server_fd);
    return 0;
}
