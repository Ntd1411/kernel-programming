/*
 * condition_var.c - Condition variables
 * 
 * Demo thread synchronization với condition variables
 * 
 * Biên dịch: gcc -o condition_var condition_var.c -pthread
 * Chạy: ./condition_var
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define MAX_QUEUE_SIZE 10

// Queue structure
typedef struct {
    int items[MAX_QUEUE_SIZE];
    int count;
    int in;
    int out;
} queue_t;

queue_t queue = {.count = 0, .in = 0, .out = 0};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;

int done = 0;

void enqueue(int item) {
    queue.items[queue.in] = item;
    queue.in = (queue.in + 1) % MAX_QUEUE_SIZE;
    queue.count++;
}

int dequeue() {
    int item = queue.items[queue.out];
    queue.out = (queue.out + 1) % MAX_QUEUE_SIZE;
    queue.count--;
    return item;
}

void *producer(void *arg) {
    int id = *(int *)arg;
    int i;
    
    for (i = 0; i < 20; i++) {
        int item = rand() % 100;
        
        pthread_mutex_lock(&mutex);
        
        // Đợi cho đến khi queue không đầy
        while (queue.count == MAX_QUEUE_SIZE) {
            printf("Producer %d: Queue đầy, đợi...\n", id);
            pthread_cond_wait(&not_full, &mutex);
        }
        
        enqueue(item);
        printf("Producer %d: Thêm %d (queue size: %d)\n", id, item, queue.count);
        
        // Signal cho consumer biết có item mới
        pthread_cond_signal(&not_empty);
        
        pthread_mutex_unlock(&mutex);
        
        usleep(rand() % 100000);
    }
    
    printf("Producer %d: Hoàn thành\n", id);
    return NULL;
}

void *consumer(void *arg) {
    int id = *(int *)arg;
    
    while (1) {
        pthread_mutex_lock(&mutex);
        
        // Đợi cho đến khi queue không rỗng hoặc done
        while (queue.count == 0 && !done) {
            printf("Consumer %d: Queue rỗng, đợi...\n", id);
            pthread_cond_wait(&not_empty, &mutex);
        }
        
        // Nếu done và queue rỗng thì thoát
        if (done && queue.count == 0) {
            pthread_mutex_unlock(&mutex);
            break;
        }
        
        int item = dequeue();
        printf("Consumer %d: Lấy %d (queue size: %d)\n", id, item, queue.count);
        
        // Signal cho producer biết có chỗ trống
        pthread_cond_signal(&not_full);
        
        pthread_mutex_unlock(&mutex);
        
        usleep(rand() % 200000);  // Tiêu thụ chậm hơn sản xuất
    }
    
    printf("Consumer %d: Hoàn thành\n", id);
    return NULL;
}

// Demo broadcast
pthread_cond_t start_cond = PTHREAD_COND_INITIALIZER;
int ready = 0;

void *worker(void *arg) {
    int id = *(int *)arg;
    
    pthread_mutex_lock(&mutex);
    
    printf("Worker %d: Chờ tín hiệu START...\n", id);
    
    // Đợi tín hiệu start
    while (!ready) {
        pthread_cond_wait(&start_cond, &mutex);
    }
    
    pthread_mutex_unlock(&mutex);
    
    printf("Worker %d: BẮT ĐẦU làm việc!\n", id);
    sleep(1);
    printf("Worker %d: Hoàn thành\n", id);
    
    return NULL;
}

void demo_broadcast() {
    pthread_t workers[5];
    int worker_ids[5];
    int i;
    
    printf("\n=== Demo pthread_cond_broadcast ===\n");
    printf("Tạo 5 workers, tất cả đợi tín hiệu START\n\n");
    
    ready = 0;
    
    // Tạo workers
    for (i = 0; i < 5; i++) {
        worker_ids[i] = i + 1;
        pthread_create(&workers[i], NULL, worker, &worker_ids[i]);
    }
    
    sleep(2);
    
    printf("\nMain: Gửi tín hiệu START đến TẤT CẢ workers (broadcast)\n\n");
    
    pthread_mutex_lock(&mutex);
    ready = 1;
    pthread_cond_broadcast(&start_cond);  // Đánh thức TẤT CẢ
    pthread_mutex_unlock(&mutex);
    
    // Đợi workers
    for (i = 0; i < 5; i++) {
        pthread_join(workers[i], NULL);
    }
}

int main() {
    pthread_t producers[3];
    pthread_t consumers[2];
    int producer_ids[3];
    int consumer_ids[2];
    int i;
    
    srand(time(NULL));
    
    printf("=== Demo Condition Variables ===\n");
    printf("Producer-Consumer với condition variables\n");
    printf("Queue size: %d\n\n", MAX_QUEUE_SIZE);
    
    // Tạo producers
    for (i = 0; i < 3; i++) {
        producer_ids[i] = i + 1;
        pthread_create(&producers[i], NULL, producer, &producer_ids[i]);
    }
    
    // Tạo consumers
    for (i = 0; i < 2; i++) {
        consumer_ids[i] = i + 1;
        pthread_create(&consumers[i], NULL, consumer, &consumer_ids[i]);
    }
    
    // Đợi producers hoàn thành
    for (i = 0; i < 3; i++) {
        pthread_join(producers[i], NULL);
    }
    
    printf("\n--- Producers hoàn thành, đánh dấu done ---\n");
    
    pthread_mutex_lock(&mutex);
    done = 1;
    pthread_cond_broadcast(&not_empty);  // Đánh thức consumers để thoát
    pthread_mutex_unlock(&mutex);
    
    // Đợi consumers hoàn thành
    for (i = 0; i < 2; i++) {
        pthread_join(consumers[i], NULL);
    }
    
    printf("\n=== Queue cuối: %d items còn lại ===\n", queue.count);
    
    // Demo broadcast
    demo_broadcast();
    
    // Cleanup
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&not_empty);
    pthread_cond_destroy(&not_full);
    pthread_cond_destroy(&start_cond);
    
    printf("\n=== Condition Variable là gì? ===\n");
    printf("Cho phép threads đợi một điều kiện xảy ra\n\n");
    printf("pthread_cond_wait(&cond, &mutex):\n");
    printf("  - Unlock mutex và đợi\n");
    printf("  - Khi được signal, lock lại mutex\n\n");
    printf("pthread_cond_signal(&cond):\n");
    printf("  - Đánh thức 1 thread đang đợi\n\n");
    printf("pthread_cond_broadcast(&cond):\n");
    printf("  - Đánh thức TẤT CẢ threads đang đợi\n");
    
    return 0;
}
