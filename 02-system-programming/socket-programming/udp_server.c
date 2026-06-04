/*
 * udp_server.c - UDP Server đơn giản
 * 
 * Server nhận gói tin UDP và gửi response
 * 
 * Biên dịch: gcc -o udp_server udp_server.c
 * Chạy: ./udp_server <port>
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
    if (argc != 2) {
        fprintf(stderr, "Sử dụng: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int port = atoi(argv[1]);
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    
    // Tạo UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    printf("UDP Socket được tạo thành công\n");
    
    // Bind
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    printf("UDP Server đang lắng nghe trên port %d\n", port);
    printf("Nhấn Ctrl+C để dừng\n\n");
    
    // Nhận và xử lý datagram
    while (1) {
        ssize_t bytes_read = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                                      (struct sockaddr*)&client_addr, &addr_len);
        
        if (bytes_read < 0) {
            perror("recvfrom");
            continue;
        }
        
        buffer[bytes_read] = '\0';
        
        printf("Nhận được từ %s:%d (%zd bytes)\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port),
               bytes_read);
        printf("Dữ liệu: %s\n", buffer);
        
        // Tạo response
        char response[BUFFER_SIZE];
        snprintf(response, BUFFER_SIZE, "Server đã nhận: %s", buffer);
        
        // Gửi response
        ssize_t sent = sendto(sockfd, response, strlen(response), 0,
                             (struct sockaddr*)&client_addr, addr_len);
        
        if (sent < 0) {
            perror("sendto");
        } else {
            printf("Đã gửi response (%zd bytes)\n\n", sent);
        }
    }
    
    close(sockfd);
    return 0;
}
