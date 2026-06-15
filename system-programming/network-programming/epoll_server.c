/*
 * epoll_server.c - Server sử dụng epoll() (Linux-specific, hiệu quả nhất)
 * 
 * epoll rất hiệu quả với số lượng lớn connections (hàng ngàn)
 * 
 * Biên dịch: gcc -o epoll_server epoll_server.c
 * Chạy: ./epoll_server <port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_EVENTS 1000
#define BUFFER_SIZE 1024

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return -1;
    }
    
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL");
        return -1;
    }
    
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Sử dụng: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int port = atoi(argv[1]);
    int server_fd, epoll_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    struct epoll_event ev, events[MAX_EVENTS];
    char buffer[BUFFER_SIZE];
    
    // Tạo socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Set non-blocking
    if (set_nonblocking(server_fd) < 0) {
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
    if (listen(server_fd, SOMAXCONN) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Epoll Server đang lắng nghe trên port %d\n", port);
    printf("Max events: %d\n", MAX_EVENTS);
    printf("Nhấn Ctrl+C để dừng\n\n");
    
    // Tạo epoll instance
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        perror("epoll_create1");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Thêm server socket vào epoll
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) < 0) {
        perror("epoll_ctl: server_fd");
        close(server_fd);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }
    
    int total_clients = 0;
    
    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds < 0) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }
        
        for (int i = 0; i < nfds; i++) {
            // Kết nối mới
            if (events[i].data.fd == server_fd) {
                while (1) {
                    int new_client = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
                    if (new_client < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        } else {
                            perror("accept");
                            break;
                        }
                    }
                    
                    printf("Kết nối mới từ %s:%d (fd=%d)\n",
                           inet_ntoa(client_addr.sin_addr),
                           ntohs(client_addr.sin_port),
                           new_client);
                    
                    if (set_nonblocking(new_client) < 0) {
                        close(new_client);
                        continue;
                    }
                    
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = new_client;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_client, &ev) < 0) {
                        perror("epoll_ctl: client");
                        close(new_client);
                        continue;
                    }
                    
                    total_clients++;
                    printf("Client được thêm. Tổng: %d clients\n", total_clients);
                    
                    const char *welcome = "Chào mừng đến Epoll Server!\n";
                    send(new_client, welcome, strlen(welcome), 0);
                }
            }
            // Dữ liệu từ client
            else {
                int client_fd = events[i].data.fd;
                
                while (1) {
                    ssize_t bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
                    
                    if (bytes_read < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }
                        perror("recv");
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                        close(client_fd);
                        total_clients--;
                        break;
                    } else if (bytes_read == 0) {
                        printf("Client (fd=%d) đã ngắt kết nối\n", client_fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                        close(client_fd);
                        total_clients--;
                        printf("Còn lại: %d clients\n", total_clients);
                        break;
                    }
                    
                    buffer[bytes_read] = '\0';
                    printf("[Client fd=%d] %.*s", client_fd, (int)bytes_read, buffer);
                    
                    // Echo back
                    ssize_t sent = send(client_fd, buffer, bytes_read, 0);
                    if (sent < 0) {
                        if (errno != EAGAIN && errno != EWOULDBLOCK) {
                            perror("send");
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                            close(client_fd);
                            total_clients--;
                            break;
                        }
                    }
                }
            }
        }
    }
    
    // Cleanup
    close(server_fd);
    close(epoll_fd);
    
    return 0;
}
