/*
 * load_balance.c - Cân bằng tải giữa các CPUs
 * 
 * Demo phân phối công việc đồng đều lên các CPU cores
 * 
 * Biên dịch: gcc -o load_balance load_balance.c -pthread -lm
 * Chạy: ./load_balance
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <string.h>

#define MAX_THREADS 32
#define WORK_SIZE 1000000

typedef struct {
    int thread_id;
    int cpu_id;
    double result;
    double elapsed_time;
} worker_data_t;

// Công việc CPU-intensive
double cpu_intensive_work(int iterations) {
    double result = 0.0;
    int i;
    
    for (i = 0; i < iterations; i++) {
        result += sqrt(i) * sin(i) * cos(i);
    }
    
    return result;
}

void *balanced_worker(void *arg) {
    worker_data_t *data = (worker_data_t *)arg;
    cpu_set_t cpuset;
    struct timespec start, end;
    
    // Pin vào CPU được gán
    CPU_ZERO(&cpuset);
    CPU_SET(data->cpu_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    
    printf("Thread %d -> CPU %d\n", data->thread_id, data->cpu_id);
    
    // Đo thời gian thực hiện
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    data->result = cpu_intensive_work(WORK_SIZE);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    data->elapsed_time = (end.tv_sec - start.tv_sec) + 
                         (end.tv_nsec - start.tv_nsec) / 1e9;
    
    printf("Thread %d (CPU %d): Hoàn thành trong %.3f giây\n", 
           data->thread_id, data->cpu_id, data->elapsed_time);
    
    return NULL;
}

void *unbalanced_worker(void *arg) {
    worker_data_t *data = (worker_data_t *)arg;
    struct timespec start, end;
    
    printf("Thread %d: Không pin (OS tự quản lý)\n", data->thread_id);
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    data->result = cpu_intensive_work(WORK_SIZE);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    data->elapsed_time = (end.tv_sec - start.tv_sec) + 
                         (end.tv_nsec - start.tv_nsec) / 1e9;
    
    printf("Thread %d: Hoàn thành trong %.3f giây\n", 
           data->thread_id, data->elapsed_time);
    
    return NULL;
}

double run_benchmark(const char *name, int num_threads, 
                     void *(*worker_func)(void *), int pin_cpus) {
    pthread_t threads[MAX_THREADS];
    worker_data_t data[MAX_THREADS];
    struct timespec start, end;
    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    int i;
    
    printf("\n=== %s ===\n", name);
    printf("Threads: %d, CPUs: %d\n\n", num_threads, num_cpus);
    
    // Khởi tạo data
    for (i = 0; i < num_threads; i++) {
        data[i].thread_id = i + 1;
        data[i].cpu_id = pin_cpus ? (i % num_cpus) : -1;
        data[i].result = 0.0;
        data[i].elapsed_time = 0.0;
    }
    
    // Bắt đầu benchmark
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Tạo threads
    for (i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, worker_func, &data[i]);
    }
    
    // Đợi hoàn thành
    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double total_time = (end.tv_sec - start.tv_sec) + 
                        (end.tv_nsec - start.tv_nsec) / 1e9;
    
    // Tính thống kê
    double min_time = data[0].elapsed_time;
    double max_time = data[0].elapsed_time;
    double sum_time = 0.0;
    
    for (i = 0; i < num_threads; i++) {
        if (data[i].elapsed_time < min_time) min_time = data[i].elapsed_time;
        if (data[i].elapsed_time > max_time) max_time = data[i].elapsed_time;
        sum_time += data[i].elapsed_time;
    }
    
    double avg_time = sum_time / num_threads;
    
    printf("\n--- Kết quả ---\n");
    printf("Tổng thời gian: %.3f giây\n", total_time);
    printf("Thread time - Min: %.3f, Max: %.3f, Avg: %.3f giây\n", 
           min_time, max_time, avg_time);
    printf("Độ lệch (Max-Min): %.3f giây (%.1f%%)\n", 
           max_time - min_time, (max_time - min_time) / avg_time * 100);
    printf("Throughput: %.2f tasks/sec\n", num_threads / total_time);
    
    return total_time;
}

// Demo với số lượng threads khác nhau
void compare_strategies() {
    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    int num_threads_list[] = {num_cpus / 2, num_cpus, num_cpus * 2, num_cpus * 4};
    int num_tests = sizeof(num_threads_list) / sizeof(int);
    int i;
    
    printf("\n=== So sánh chiến lược với số threads khác nhau ===\n");
    
    for (i = 0; i < num_tests; i++) {
        int num_threads = num_threads_list[i];
        
        if (num_threads > MAX_THREADS) {
            num_threads = MAX_THREADS;
        }
        
        printf("\n" "========================================\n");
        printf("Test với %d threads trên %d CPUs\n", num_threads, num_cpus);
        printf("========================================\n");
        
        double pinned_time = run_benchmark(
            "Chiến lược 1: Pin threads vào CPUs", 
            num_threads, balanced_worker, 1
        );
        
        sleep(2);
        
        double unpinned_time = run_benchmark(
            "Chiến lược 2: Để OS tự cân bằng", 
            num_threads, unbalanced_worker, 0
        );
        
        printf("\n>>> So sánh: ");
        if (pinned_time < unpinned_time) {
            printf("Pin nhanh hơn %.2fx\n", unpinned_time / pinned_time);
        } else {
            printf("OS tự cân bằng nhanh hơn %.2fx\n", pinned_time / unpinned_time);
        }
        
        sleep(2);
    }
}

// Demo round-robin assignment
void demo_round_robin() {
    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    int num_threads = num_cpus * 2;
    int i;
    
    printf("\n=== Demo Round-Robin Assignment ===\n");
    printf("Gán %d threads lên %d CPUs theo kiểu round-robin\n\n", 
           num_threads, num_cpus);
    
    printf("Mapping:\n");
    for (i = 0; i < num_threads; i++) {
        int cpu = i % num_cpus;
        printf("Thread %d -> CPU %d\n", i + 1, cpu);
    }
    
    run_benchmark("Round-Robin", num_threads, balanced_worker, 1);
}

int main() {
    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    
    printf("=== Load Balancing Demo ===\n");
    printf("System: %d CPUs\n", num_cpus);
    
    if (num_cpus < 2) {
        printf("Cần ít nhất 2 CPUs để demo có ý nghĩa\n");
        return 1;
    }
    
    demo_round_robin();
    
    compare_strategies();
    
    printf("\n=== Kết luận ===\n");
    printf("Pinning threads:\n");
    printf("+ Tốt khi: Số threads ≈ số CPUs, workload đồng đều\n");
    printf("+ Giảm cache misses, context switches\n");
    printf("+ Predictable performance\n");
    printf("\n");
    printf("OS auto-balancing:\n");
    printf("+ Tốt khi: Số threads >> CPUs, workload không đồng đều\n");
    printf("+ Linh hoạt hơn khi có nhiều processes khác\n");
    printf("+ Tự động adapt với system load\n");
    
    return 0;
}
