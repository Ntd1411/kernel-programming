/*
 * tcp_client.c - TCP Client đơn giản
 * 
 * Client kết nối đến TCP server và gửi/nhận dữ liệu
 * 
 * Biên dịch: gcc -o tcp_client tcp_client.c
 * Chạy: ./tcp_client <server_ip> <port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Sử dụng: %s <server_ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    char *server_ip = argv[1];
    int port = atoi(argv[2]);
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    
    // Tạo socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    printf("Socket được tạo thành công\n");
    
    // Cấu hình server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Địa chỉ IP không hợp lệ: %s\n", server_ip);
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    // Kết nối đến server
    printf("Đang kết nối đến %s:%d...\n", server_ip, port);
    
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    printf("Đã kết nối đến server!\n\n");
    
    // Nhận welcome message
    ssize_t bytes_read = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("Server: %s\n", buffer);
    }
    
    printf("Nhập tin nhắn (gõ 'quit' để thoát):\n");
    
    // Gửi và nhận dữ liệu
    while (1) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            break;
        }
        
        // Kiểm tra lệnh quit
        if (strncmp(buffer, "quit", 4) == 0) {
            printf("Ngắt kết nối...\n");
            break;
        }
        
        // Gửi dữ liệu
        ssize_t len = strlen(buffer);
        if (send(sockfd, buffer, len, 0) < 0) {
            perror("send");
            break;
        }
        
        // Nhận echo
        bytes_read = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_read < 0) {
            perror("recv");
            break;
        } else if (bytes_read == 0) {
            printf("Server đã ngắt kết nối\n");
            break;
        }
        
        buffer[bytes_read] = '\0';
        printf("Echo: %s", buffer);
    }
    
    close(sockfd);
    printf("Đã đóng kết nối\n");
    
    return 0;
}
