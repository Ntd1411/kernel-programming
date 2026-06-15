/*
 * spinlock.c - Spinlocks
 * 
 * Spinlock: Busy-wait lock (không sleep, liên tục kiểm tra)
 * Hiệu quả cho critical sections rất ngắn
 * 
 * Biên dịch: gcc -o spinlock spinlock.c -pthread
 * Chạy: ./spinlock
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_THREADS 4
#define ITERATIONS 100000

pthread_spinlock_t spinlock;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

long long shared_counter = 0;

// Thread dùng spinlock
void *spinlock_thread(void *arg) {
    int id = *(int *)arg;
    int i;
    
    for (i = 0; i < ITERATIONS; i++) {
        pthread_spin_lock(&spinlock);
        shared_counter++;
        pthread_spin_unlock(&spinlock);
    }
    
    printf("Spinlock Thread %d: Hoàn thành\n", id);
    return NULL;
}

// Thread dùng mutex
void *mutex_thread(void *arg) {
    int id = *(int *)arg;
    int i;
    
    for (i = 0; i < ITERATIONS; i++) {
        pthread_mutex_lock(&mutex);
        shared_counter++;
        pthread_mutex_unlock(&mutex);
    }
    
    printf("Mutex Thread %d: Hoàn thành\n", id);
    return NULL;
}

double benchmark(const char *name, void *(*thread_func)(void *)) {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    int i;
    struct timespec start, end;
    
    shared_counter = 0;
    
    printf("\n=== Benchmark: %s ===\n", name);
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Tạo threads
    for (i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i + 1;
        pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]);
    }
    
    // Đợi hoàn thành
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double elapsed = (end.tv_sec - start.tv_sec) + 
                     (end.tv_nsec - start.tv_nsec) / 1e9;
    
    printf("Counter: %lld (expected: %d)\n", shared_counter, NUM_THREADS * ITERATIONS);
    printf("Thời gian: %.3f giây\n", elapsed);
    printf("Throughput: %.0f ops/sec\n", (NUM_THREADS * ITERATIONS) / elapsed);
    
    return elapsed;
}

// Demo spinlock với contention cao
pthread_spinlock_t high_contention_lock;
int contention_counter = 0;

void *high_contention_thread(void *arg) {
    int id = *(int *)arg;
    int i;
    
    for (i = 0; i < 10000; i++) {
        pthread_spin_lock(&high_contention_lock);
        
        // Critical section rất ngắn
        contention_counter++;
        
        pthread_spin_unlock(&high_contention_lock);
    }
    
    return NULL;
}

// Demo spinlock với contention thấp
pthread_spinlock_t low_contention_lock;
int low_contention_counter = 0;

void *low_contention_thread(void *arg) {
    int id = *(int *)arg;
    int i;
    
    for (i = 0; i < 1000; i++) {
        pthread_spin_lock(&low_contention_lock);
        
        // Critical section rất ngắn
        low_contention_counter++;
        
        pthread_spin_unlock(&low_contention_lock);
        
        // Làm công việc khác (không giữ lock)
        usleep(10);
    }
    
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    int i;
    double spinlock_time, mutex_time;
    
    printf("=== Demo Spinlock vs Mutex ===\n");
    printf("Threads: %d, Iterations per thread: %d\n", NUM_THREADS, ITERATIONS);
    
    // Khởi tạo spinlock
    pthread_spin_init(&spinlock, PTHREAD_PROCESS_PRIVATE);
    
    // Benchmark spinlock
    spinlock_time = benchmark("Spinlock (critical section cực ngắn)", spinlock_thread);
    
    // Benchmark mutex
    mutex_time = benchmark("Mutex (critical section cực ngắn)", mutex_thread);
    
    printf("\n=== So sánh ===\n");
    if (spinlock_time < mutex_time) {
        printf("Spinlock nhanh hơn %.2fx\n", mutex_time / spinlock_time);
    } else {
        printf("Mutex nhanh hơn %.2fx\n", spinlock_time / mutex_time);
    }
    
    // Demo high contention
    printf("\n=== High Contention (nhiều threads tranh giành lock) ===\n");
    pthread_spin_init(&high_contention_lock, PTHREAD_PROCESS_PRIVATE);
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i + 1;
        pthread_create(&threads[i], NULL, high_contention_thread, &thread_ids[i]);
    }
    
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double high_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    
    printf("Counter: %d, Thời gian: %.3f giây\n", contention_counter, high_time);
    
    // Demo low contention
    printf("\n=== Low Contention (ít tranh giành lock) ===\n");
    pthread_spin_init(&low_contention_lock, PTHREAD_PROCESS_PRIVATE);
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i + 1;
        pthread_create(&threads[i], NULL, low_contention_thread, &thread_ids[i]);
    }
    
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double low_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    
    printf("Counter: %d, Thời gian: %.3f giây\n", low_contention_counter, low_time);
    
    // Cleanup
    pthread_spin_destroy(&spinlock);
    pthread_spin_destroy(&high_contention_lock);
    pthread_spin_destroy(&low_contention_lock);
    pthread_mutex_destroy(&mutex);
    
    printf("\n=== Spinlock vs Mutex ===\n");
    printf("\nSpinlock:\n");
    printf("+ Rất nhanh cho critical section cực ngắn\n");
    printf("+ Không context switch\n");
    printf("- Lãng phí CPU (busy-wait)\n");
    printf("- Không phù hợp với critical section dài\n");
    printf("- Có thể gây priority inversion\n");
    
    printf("\nMutex:\n");
    printf("+ Không lãng phí CPU (thread sleep)\n");
    printf("+ Phù hợp với critical section dài\n");
    printf("- Overhead của context switch\n");
    printf("- Chậm hơn spinlock cho critical section ngắn\n");
    
    printf("\nKhi nào dùng spinlock?\n");
    printf("- Critical section < 100 CPU cycles\n");
    printf("- Lock được giữ trong thời gian rất ngắn\n");
    printf("- Kernel mode code\n");
    printf("- Real-time systems\n");
    
    return 0;
}
