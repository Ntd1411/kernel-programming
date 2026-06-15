/*
 * semaphore.c - Semaphore synchronization
 * 
 * Demo Producer-Consumer với bounded buffer
 * 
 * Biên dịch: gcc -o semaphore semaphore.c -pthread
 * Chạy: ./semaphore
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define BUFFER_SIZE 5
#define NUM_PRODUCERS 3
#define NUM_CONSUMERS 2
#define ITEMS_PER_PRODUCER 5

int buffer[BUFFER_SIZE];
int in = 0;   // Vị trí producer ghi
int out = 0;  // Vị trí consumer đọc

sem_t empty;  // Số slot trống
sem_t full;   // Số slot đầy
pthread_mutex_t mutex;  // Bảo vệ buffer

int produced_count = 0;
int consumed_count = 0;

void print_buffer() {
    int i;
    printf("Buffer: [");
    for (i = 0; i < BUFFER_SIZE; i++) {
        if (i > 0) printf(", ");
        if ((out <= in && i >= out && i < in) || 
            (out > in && (i >= out || i < in))) {
            printf("%d", buffer[i]);
        } else {
            printf("_");
        }
    }
    printf("]\n");
}

void *producer(void *arg) {
    int id = *(int *)arg;
    int i;
    
    for (i = 0; i < ITEMS_PER_PRODUCER; i++) {
        int item = rand() % 100;
        
        printf("Producer %d: Sản xuất item %d\n", id, item);
        
        sem_wait(&empty);  // Đợi có slot trống
        pthread_mutex_lock(&mutex);
        
        // Thêm vào buffer
        buffer[in] = item;
        printf("Producer %d: Thêm %d vào buffer[%d]\n", id, item, in);
        in = (in + 1) % BUFFER_SIZE;
        produced_count++;
        
        print_buffer();
        
        pthread_mutex_unlock(&mutex);
        sem_post(&full);  // Tăng số slot đầy
        
        sleep(rand() % 2);  // Nghỉ ngẫu nhiên
    }
    
    printf("Producer %d: Hoàn thành\n", id);
    return NULL;
}

void *consumer(void *arg) {
    int id = *(int *)arg;
    int total_items = NUM_PRODUCERS * ITEMS_PER_PRODUCER;
    
    while (consumed_count < total_items) {
        sem_wait(&full);  // Đợi có item
        pthread_mutex_lock(&mutex);
        
        if (consumed_count >= total_items) {
            pthread_mutex_unlock(&mutex);
            sem_post(&full);
            break;
        }
        
        // Lấy từ buffer
        int item = buffer[out];
        printf("Consumer %d: Lấy %d từ buffer[%d]\n", id, item, out);
        out = (out + 1) % BUFFER_SIZE;
        consumed_count++;
        
        print_buffer();
        
        pthread_mutex_unlock(&mutex);
        sem_post(&empty);  // Tăng số slot trống
        
        printf("Consumer %d: Tiêu thụ item %d\n", id, item);
        sleep(rand() % 3);  // Tiêu thụ lâu hơn sản xuất
    }
    
    printf("Consumer %d: Hoàn thành\n", id);
    return NULL;
}

int main() {
    pthread_t producers[NUM_PRODUCERS];
    pthread_t consumers[NUM_CONSUMERS];
    int producer_ids[NUM_PRODUCERS];
    int consumer_ids[NUM_CONSUMERS];
    int i;
    
    srand(time(NULL));
    
    printf("=== Producer-Consumer với Semaphore ===\n");
    printf("Buffer size: %d\n", BUFFER_SIZE);
    printf("Producers: %d, Consumers: %d\n", NUM_PRODUCERS, NUM_CONSUMERS);
    printf("Items per producer: %d\n\n", ITEMS_PER_PRODUCER);
    
    // Khởi tạo semaphores
    sem_init(&empty, 0, BUFFER_SIZE);  // Ban đầu tất cả slots trống
    sem_init(&full, 0, 0);             // Không có item nào
    pthread_mutex_init(&mutex, NULL);
    
    // Tạo producer threads
    for (i = 0; i < NUM_PRODUCERS; i++) {
        producer_ids[i] = i + 1;
        if (pthread_create(&producers[i], NULL, producer, &producer_ids[i]) != 0) {
            perror("pthread_create producer failed");
            exit(EXIT_FAILURE);
        }
    }
    
    // Tạo consumer threads
    for (i = 0; i < NUM_CONSUMERS; i++) {
        consumer_ids[i] = i + 1;
        if (pthread_create(&consumers[i], NULL, consumer, &consumer_ids[i]) != 0) {
            perror("pthread_create consumer failed");
            exit(EXIT_FAILURE);
        }
    }
    
    // Đợi producers hoàn thành
    for (i = 0; i < NUM_PRODUCERS; i++) {
        pthread_join(producers[i], NULL);
    }
    
    printf("\n--- Tất cả producers đã hoàn thành ---\n\n");
    
    // Đợi consumers hoàn thành
    for (i = 0; i < NUM_CONSUMERS; i++) {
        pthread_join(consumers[i], NULL);
    }
    
    printf("\n=== Kết quả ===\n");
    printf("Tổng sản xuất: %d items\n", produced_count);
    printf("Tổng tiêu thụ: %d items\n", consumed_count);
    
    if (produced_count == consumed_count) {
        printf("✓ Đúng: Số lượng khớp nhau\n");
    } else {
        printf("✗ Lỗi: Số lượng không khớp!\n");
    }
    
    // Cleanup
    sem_destroy(&empty);
    sem_destroy(&full);
    pthread_mutex_destroy(&mutex);
    
    printf("\n=== Semaphore là gì? ===\n");
    printf("Semaphore: Biến đếm để kiểm soát truy cập tài nguyên\n");
    printf("- sem_wait(): Giảm counter, block nếu counter = 0\n");
    printf("- sem_post(): Tăng counter, đánh thức thread đang đợi\n");
    printf("\nBinary semaphore (0/1) ~ Mutex\n");
    printf("Counting semaphore: Giới hạn số lượng truy cập đồng thời\n");
    
    return 0;
}
