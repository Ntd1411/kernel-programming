/*
 * udp_client.c - UDP Client đơn giản
 * 
 * Client gửi datagram UDP đến server
 * 
 * Biên dịch: gcc -o udp_client udp_client.c
 * Chạy: ./udp_client <server_ip_or_hostname> <port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Sử dụng: %s <server_ip_or_hostname> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    char *server_addr = argv[1];
    int port = atoi(argv[2]);
    int sockfd;
    struct sockaddr_in server_sockaddr;
    socklen_t addr_len = sizeof(server_sockaddr);
    char buffer[BUFFER_SIZE];
    struct hostent *server_host;
    
    // Tạo UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    printf("UDP Socket được tạo thành công\n");
    
    // Cấu hình server address
    memset(&server_sockaddr, 0, sizeof(server_sockaddr));
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(port);
    
    // Thử chuyển đổi IP address trực tiếp
    if (inet_pton(AF_INET, server_addr, &server_sockaddr.sin_addr) <= 0) {
        // Không phải IP address, thử resolve hostname
        printf("Đang resolve hostname: %s\n", server_addr);
        server_host = gethostbyname(server_addr);
        
        if (server_host == NULL) {
            fprintf(stderr, "Không thể resolve hostname: %s\n", server_addr);
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        
        memcpy(&server_sockaddr.sin_addr.s_addr, server_host->h_addr, server_host->h_length);
        printf("Resolved: %s -> %s\n", server_addr, inet_ntoa(server_sockaddr.sin_addr));
    }
    
    printf("Sẵn sàng gửi tin nhắn đến %s:%d\n", server_addr, port);
    printf("Nhập tin nhắn (gõ 'quit' để thoát):\n\n");
    
    while (1) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            break;
        }
        
        // Kiểm tra lệnh quit
        if (strncmp(buffer, "quit", 4) == 0) {
            printf("Thoát...\n");
            break;
        }
        
        // Gửi datagram
        ssize_t sent = sendto(sockfd, buffer, strlen(buffer), 0,
                             (struct sockaddr*)&server_addr, addr_len);
        
        if (sent < 0) {
            perror("sendto");
            continue;
        }
        
        printf("Đã gửi %zd bytes\n", sent);
        
        // Nhận response (với timeout)
        struct timeval tv;
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        
        ssize_t bytes_read = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0,
                                      (struct sockaddr*)&server_addr, &addr_len);
        
        if (bytes_read < 0) {
            printf("Không nhận được response từ server\n\n");
            continue;
        }
        
        buffer[bytes_read] = '\0';
        printf("Response: %s\n", buffer);
    }
    
    close(sockfd);
    return 0;
}
