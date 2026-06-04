/*
 * mutex_basic.c - Mutex locks cơ bản
 * 
 * Demo race condition và cách giải quyết bằng mutex
 * 
 * Biên dịch: gcc -o mutex_basic mutex_basic.c -pthread
 * Chạy: ./mutex_basic
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 5
#define ITERATIONS 100000

// Biến chia sẻ giữa các threads
int shared_counter = 0;

// Mutex để bảo vệ biến chia sẻ
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Thread không có mutex (race condition)
void *unsafe_increment(void *arg) {
    int i;
    int thread_id = *(int *)arg;
    
    for (i = 0; i < ITERATIONS; i++) {
        shared_counter++;  // KHÔNG AN TOÀN!
    }
    
    printf("Thread %d: Hoàn thành (unsafe)\n", thread_id);
    return NULL;
}

// Thread có mutex (an toàn)
void *safe_increment(void *arg) {
    int i;
    int thread_id = *(int *)arg;
    
    for (i = 0; i < ITERATIONS; i++) {
        pthread_mutex_lock(&mutex);
        shared_counter++;  // AN TOÀN
        pthread_mutex_unlock(&mutex);
    }
    
    printf("Thread %d: Hoàn thành (safe)\n", thread_id);
    return NULL;
}

// Thread với critical section lớn hơn
void *optimized_increment(void *arg) {
    int i;
    int thread_id = *(int *)arg;
    int local_counter = 0;
    
    // Tính toán local trước
    for (i = 0; i < ITERATIONS; i++) {
        local_counter++;
    }
    
    // Lock một lần duy nhất
    pthread_mutex_lock(&mutex);
    shared_counter += local_counter;
    pthread_mutex_unlock(&mutex);
    
    printf("Thread %d: Hoàn thành (optimized)\n", thread_id);
    return NULL;
}

void run_test(const char *test_name, void *(*thread_func)(void *)) {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    int i;
    
    // Reset counter
    shared_counter = 0;
    
    printf("\n=== %s ===\n", test_name);
    printf("Tạo %d threads, mỗi thread tăng counter %d lần\n", NUM_THREADS, ITERATIONS);
    printf("Kết quả mong đợi: %d\n\n", NUM_THREADS * ITERATIONS);
    
    // Tạo threads
    for (i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i + 1;
        if (pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]) != 0) {
            perror("pthread_create failed");
            exit(EXIT_FAILURE);
        }
    }
    
    // Đợi threads hoàn thành
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\nKết quả thực tế: %d\n", shared_counter);
    
    if (shared_counter == NUM_THREADS * ITERATIONS) {
        printf("✓ ĐÚNG: Không có race condition\n");
    } else {
        printf("✗ SAI: Race condition xảy ra! Mất %d lần tăng\n", 
               NUM_THREADS * ITERATIONS - shared_counter);
    }
}

int main() {
    printf("=== Demo Mutex Locks ===\n");
    printf("Minh họa race condition và cách giải quyết\n");
    
    // Test 1: Không dùng mutex (race condition)
    run_test("Test 1: KHÔNG dùng mutex (UNSAFE)", unsafe_increment);
    
    sleep(1);
    
    // Test 2: Dùng mutex (an toàn nhưng chậm)
    run_test("Test 2: Dùng mutex (SAFE nhưng chậm)", safe_increment);
    
    sleep(1);
    
    // Test 3: Tối ưu với local counter
    run_test("Test 3: Tối ưu với local counter (SAFE và nhanh)", optimized_increment);
    
    // Cleanup
    pthread_mutex_destroy(&mutex);
    
    printf("\n=== Bài học ===\n");
    printf("1. Race condition: Nhiều threads truy cập shared data không đồng bộ\n");
    printf("2. Mutex: Đảm bảo chỉ 1 thread vào critical section\n");
    printf("3. Tối ưu: Giảm thời gian giữ lock bằng cách tính toán local\n");
    
    return 0;
}
