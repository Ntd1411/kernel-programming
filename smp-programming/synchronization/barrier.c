/*
 * barrier.c - Thread barriers
 * 
 * Barrier: Điểm đồng bộ cho nhiều threads
 * Tất cả threads phải đến barrier trước khi tiếp tục
 * 
 * Biên dịch: gcc -o barrier barrier.c -pthread
 * Chạy: ./barrier
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_THREADS 5
#define NUM_PHASES 3

pthread_barrier_t barrier;

void *worker_thread(void *arg) {
    int id = *(int *)arg;
    int phase;
    
    for (phase = 1; phase <= NUM_PHASES; phase++) {
        printf("Thread %d: Phase %d - Bắt đầu công việc\n", id, phase);
        
        // Giả lập công việc với thời gian ngẫu nhiên
        sleep(rand() % 3 + 1);
        
        printf("Thread %d: Phase %d - Hoàn thành, đợi tại barrier...\n", id, phase);
        
        // Đợi tất cả threads đến barrier
        int rc = pthread_barrier_wait(&barrier);
        
        if (rc == PTHREAD_BARRIER_SERIAL_THREAD) {
            // Thread cuối cùng đến barrier
            printf("\n>>> Thread %d: Barrier released! Tất cả threads tiếp tục phase %d >>>\n\n", 
                   id, phase + 1);
        }
    }
    
    printf("Thread %d: Hoàn thành tất cả phases\n", id);
    return NULL;
}

// Demo parallel processing với barriers
#define ARRAY_SIZE 100
#define CHUNK_SIZE 20

int data_array[ARRAY_SIZE];
int processed_array[ARRAY_SIZE];

void *parallel_processor(void *arg) {
    int id = *(int *)arg;
    int start = id * CHUNK_SIZE;
    int end = start + CHUNK_SIZE;
    int i;
    
    printf("Processor %d: Xử lý phần tử [%d-%d]\n", id, start, end - 1);
    
    // Phase 1: Đọc và validate
    printf("Processor %d: Phase 1 - Validate\n", id);
    for (i = start; i < end; i++) {
        if (data_array[i] < 0) {
            printf("Processor %d: Lỗi! data[%d] = %d\n", id, i, data_array[i]);
        }
    }
    pthread_barrier_wait(&barrier);
    
    // Phase 2: Transform
    printf("Processor %d: Phase 2 - Transform\n", id);
    for (i = start; i < end; i++) {
        processed_array[i] = data_array[i] * 2;
    }
    pthread_barrier_wait(&barrier);
    
    // Phase 3: Verify
    printf("Processor %d: Phase 3 - Verify\n", id);
    for (i = start; i < end; i++) {
        if (processed_array[i] != data_array[i] * 2) {
            printf("Processor %d: Lỗi verify tại index %d\n", id, i);
        }
    }
    pthread_barrier_wait(&barrier);
    
    printf("Processor %d: Hoàn thành\n", id);
    return NULL;
}

void demo_parallel_processing() {
    pthread_t threads[5];
    int thread_ids[5];
    int i;
    
    printf("\n=== Demo Parallel Processing với Barriers ===\n");
    printf("Xử lý mảng %d phần tử song song với %d threads\n\n", ARRAY_SIZE, 5);
    
    // Khởi tạo dữ liệu
    for (i = 0; i < ARRAY_SIZE; i++) {
        data_array[i] = i + 1;
    }
    
    pthread_barrier_init(&barrier, NULL, 5);
    
    // Tạo processor threads
    for (i = 0; i < 5; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, parallel_processor, &thread_ids[i]);
    }
    
    // Đợi hoàn thành
    for (i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\n--- Kiểm tra kết quả ---\n");
    printf("data_array[0..4]: %d %d %d %d %d\n", 
           data_array[0], data_array[1], data_array[2], data_array[3], data_array[4]);
    printf("processed_array[0..4]: %d %d %d %d %d\n",
           processed_array[0], processed_array[1], processed_array[2], processed_array[3], processed_array[4]);
    
    pthread_barrier_destroy(&barrier);
}

int main() {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    int i;
    
    srand(time(NULL));
    
    printf("=== Demo Thread Barrier ===\n");
    printf("Tạo %d threads, mỗi thread có %d phases\n", NUM_THREADS, NUM_PHASES);
    printf("Tất cả threads phải hoàn thành phase trước khi tiếp tục phase sau\n\n");
    
    // Khởi tạo barrier với số lượng threads
    pthread_barrier_init(&barrier, NULL, NUM_THREADS);
    
    // Tạo threads
    for (i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i + 1;
        if (pthread_create(&threads[i], NULL, worker_thread, &thread_ids[i]) != 0) {
            perror("pthread_create failed");
            exit(EXIT_FAILURE);
        }
    }
    
    // Đợi tất cả threads
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    pthread_barrier_destroy(&barrier);
    
    // Demo parallel processing
    demo_parallel_processing();
    
    printf("\n=== Barrier là gì? ===\n");
    printf("Barrier: Điểm đồng bộ cho nhiều threads\n");
    printf("- pthread_barrier_init(&barrier, NULL, count)\n");
    printf("- pthread_barrier_wait(&barrier)\n");
    printf("  → Block cho đến khi 'count' threads gọi wait()\n");
    printf("  → Thread cuối cùng nhận PTHREAD_BARRIER_SERIAL_THREAD\n");
    printf("- pthread_barrier_destroy(&barrier)\n\n");
    printf("Use cases:\n");
    printf("- Pipeline processing với nhiều stages\n");
    printf("- Parallel algorithms cần synchronization points\n");
    printf("- MapReduce-style computations\n");
    
    return 0;
}
