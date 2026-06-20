/*
 * echo_client.c - Echo Client cho Echo Server
 * 
 * Client kết nối đến echo server và gửi/nhận tin nhắn
 * 
 * Biên dịch: gcc -o echo_client echo_client.c
 * Chạy: ./echo_client <server_ip_or_hostname> <port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#define BUFFER_SIZE 1024

void print_usage(const char *prog_name) {
    fprintf(stderr, "Sử dụng: %s <server_ip_or_hostname> <port>\n", prog_name);
    fprintf(stderr, "\nVí dụ:\n");
    fprintf(stderr, "  %s localhost 8080\n", prog_name);
    fprintf(stderr, "  %s 127.0.0.1 8080\n", prog_name);
    fprintf(stderr, "  %s google.com 80\n", prog_name);
}

int connect_to_server(const char *server_addr, int port) {
    int sockfd;
    struct sockaddr_in server_sockaddr;
    struct hostent *server_host;
    
    /* Tạo TCP socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    
    printf("Socket được tạo thành công (fd=%d)\n", sockfd);
    
    /* Cấu hình server address */
    memset(&server_sockaddr, 0, sizeof(server_sockaddr));
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(port);
    
    /* Thử chuyển đổi IP address trực tiếp */
    if (inet_pton(AF_INET, server_addr, &server_sockaddr.sin_addr) <= 0) {
        /* Không phải IP address, thử resolve hostname */
        printf("Đang resolve hostname: %s\n", server_addr);
        server_host = gethostbyname(server_addr);
        
        if (server_host == NULL) {
            fprintf(stderr, "Lỗi: Không thể resolve hostname: %s\n", server_addr);
            close(sockfd);
            return -1;
        }
        
        memcpy(&server_sockaddr.sin_addr.s_addr, server_host->h_addr, server_host->h_length);
        printf("Resolved: %s -> %s\n", server_addr, inet_ntoa(server_sockaddr.sin_addr));
    }
    
    /* Kết nối đến server */
    printf("Đang kết nối đến %s:%d...\n", server_addr, port);
    
    if (connect(sockfd, (struct sockaddr*)&server_sockaddr, sizeof(server_sockaddr)) < 0) {
        perror("connect");
        fprintf(stderr, "Không thể kết nối đến server %s:%d\n", server_addr, port);
        fprintf(stderr, "Hãy đảm bảo server đang chạy!\n");
        close(sockfd);
        return -1;
    }
    
    printf("Đã kết nối đến Echo Server!\n");
    
    return sockfd;
}

void run_echo_client(int sockfd) {
    char send_buffer[BUFFER_SIZE];
    char recv_buffer[BUFFER_SIZE];
    ssize_t bytes_sent, bytes_received;
    
    /* Nhận welcome message từ server */
    bytes_received = recv(sockfd, recv_buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        recv_buffer[bytes_received] = '\0';
        printf("\n%s", recv_buffer);
    }
    
    printf("===============================================\n");
    printf("Echo Client đã sẵn sàng!\n");
    printf("Nhập tin nhắn và nhấn Enter để gửi\n");
    printf("Gõ 'quit' hoặc 'exit' để thoát\n");
    printf("Nhấn Ctrl+D (EOF) để ngắt kết nối\n");
    printf("===============================================\n\n");
    
    /* Vòng lặp gửi/nhận */
    while (1) {
        printf("Bạn: ");
        fflush(stdout);
        
        /* Đọc input từ user */
        if (fgets(send_buffer, BUFFER_SIZE, stdin) == NULL) {
            /* EOF hoặc lỗi đọc */
            if (feof(stdin)) {
                printf("\n\nĐã nhận EOF. Đang ngắt kết nối...\n");
            } else {
                perror("\nLỗi đọc input");
            }
            break;
        }
        
        /* Xóa newline ở cuối nếu có */
        size_t len = strlen(send_buffer);
        if (len > 0 && send_buffer[len - 1] == '\n') {
            send_buffer[len - 1] = '\n';  /* Giữ newline để server dễ đọc */
        }
        
        /* Kiểm tra lệnh thoát */
        if (strncmp(send_buffer, "quit", 4) == 0 || 
            strncmp(send_buffer, "exit", 4) == 0) {
            printf("Đang ngắt kết nối...\n");
            break;
        }
        
        /* Bỏ qua dòng trống */
        if (len <= 1) {
            continue;
        }
        
        /* Gửi dữ liệu đến server */
        bytes_sent = send(sockfd, send_buffer, len, 0);
        if (bytes_sent < 0) {
            perror("send");
            fprintf(stderr, "Lỗi khi gửi dữ liệu. Ngắt kết nối.\n");
            break;
        }
        
        if (bytes_sent == 0) {
            fprintf(stderr, "Server đã đóng kết nối.\n");
            break;
        }
        
        /* Nhận echo từ server */
        bytes_received = recv(sockfd, recv_buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received < 0) {
            perror("recv");
            fprintf(stderr, "Lỗi khi nhận dữ liệu. Ngắt kết nối.\n");
            break;
        }
        
        if (bytes_received == 0) {
            fprintf(stderr, "\nServer đã ngắt kết nối.\n");
            break;
        }
        
        /* Hiển thị echo từ server */
        recv_buffer[bytes_received] = '\0';
        printf("Echo: %s", recv_buffer);
    }
}

int main(int argc, char *argv[]) {
    int sockfd;
    char *server_addr;
    int port;
    
    /* Kiểm tra arguments */
    if (argc != 3) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    
    server_addr = argv[1];
    port = atoi(argv[2]);
    
    /* Validate port */
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Lỗi: Port không hợp lệ: %d\n", port);
        fprintf(stderr, "Port phải nằm trong khoảng 1-65535\n");
        exit(EXIT_FAILURE);
    }
    
    printf("=== Echo Client ===\n");
    printf("Server: %s\n", server_addr);
    printf("Port: %d\n\n", port);
    
    /* Kết nối đến server */
    sockfd = connect_to_server(server_addr, port);
    if (sockfd < 0) {
        exit(EXIT_FAILURE);
    }
    
    /* Chạy echo client */
    run_echo_client(sockfd);
    
    /* Đóng kết nối */
    printf("\nĐóng kết nối...\n");
    close(sockfd);
    printf("Đã ngắt kết nối. Tạm biệt!\n");
    
    return 0;
}
