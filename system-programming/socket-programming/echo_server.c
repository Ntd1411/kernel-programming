/*
 * echo_server.c - Multi-threaded Echo Server
 * 
 * Server xử lý nhiều client đồng thời bằng threads
 * 
 * Biên dịch: gcc -o echo_server echo_server.c -lpthread
 * Chạy: ./echo_server <port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100

typedef struct {
    int client_fd;
    struct sockaddr_in client_addr;
    int client_id;
} client_info_t;

int client_count = 0;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg) {
    client_info_t *client = (client_info_t *)arg;
    char buffer[BUFFER_SIZE];
    
    printf("\n[Thread %lu] Xử lý client #%d từ %s:%d\n",
           pthread_self(),
           client->client_id,
           inet_ntoa(client->client_addr.sin_addr),
           ntohs(client->client_addr.sin_port));
    
    // Gửi welcome message
    char welcome[BUFFER_SIZE];
    snprintf(welcome, BUFFER_SIZE, 
             "Chào mừng đến Echo Server! Bạn là client #%d\n",
             client->client_id);
    send(client->client_fd, welcome, strlen(welcome), 0);
    
    // Echo loop
    while (1) {
        ssize_t bytes_read = recv(client->client_fd, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_read <= 0) {
            if (bytes_read == 0) {
                printf("[Client #%d] Đã ngắt kết nối\n", client->client_id);
            } else {
                perror("recv");
            }
            break;
        }
        
        buffer[bytes_read] = '\0';
        printf("[Client #%d] %s", client->client_id, buffer);
        
        // Echo back
        if (send(client->client_fd, buffer, bytes_read, 0) < 0) {
            perror("send");
            break;
        }
    }
    
    close(client->client_fd);
    
    pthread_mutex_lock(&count_mutex);
    client_count--;
    printf("[Client #%d] Đã đóng. Còn lại: %d clients\n", 
           client->client_id, client_count);
    pthread_mutex_unlock(&count_mutex);
    
    free(client);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Sử dụng: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int port = atoi(argv[1]);
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int next_client_id = 1;
    
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
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Echo Server đang lắng nghe trên port %d\n", port);
    printf("Số lượng clients tối đa: %d\n", MAX_CLIENTS);
    printf("Nhấn Ctrl+C để dừng\n\n");
    
    // Accept loop
    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }
        
        pthread_mutex_lock(&count_mutex);
        if (client_count >= MAX_CLIENTS) {
            printf("Đã đạt số lượng clients tối đa. Từ chối kết nối.\n");
            close(client_fd);
            pthread_mutex_unlock(&count_mutex);
            continue;
        }
        client_count++;
        pthread_mutex_unlock(&count_mutex);
        
        // Chuẩn bị thông tin client
        client_info_t *client = malloc(sizeof(client_info_t));
        if (client == NULL) {
            perror("malloc");
            close(client_fd);
            pthread_mutex_lock(&count_mutex);
            client_count--;
            pthread_mutex_unlock(&count_mutex);
            continue;
        }
        
        client->client_fd = client_fd;
        client->client_addr = client_addr;
        client->client_id = next_client_id++;
        
        // Tạo thread xử lý client
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, client) != 0) {
            perror("pthread_create");
            free(client);
            close(client_fd);
            pthread_mutex_lock(&count_mutex);
            client_count--;
            pthread_mutex_unlock(&count_mutex);
            continue;
        }
        
        // Detach thread
        pthread_detach(thread);
        
        printf("Đã tạo thread cho client #%d. Tổng: %d clients\n",
               client->client_id, client_count);
    }
    
    close(server_fd);
    pthread_mutex_destroy(&count_mutex);
    
    return 0;
}
