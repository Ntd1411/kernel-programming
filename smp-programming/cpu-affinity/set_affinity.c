/*
 * set_affinity.c - Gán thread vào CPU cụ thể
 * 
 * Demo CPU affinity để kiểm soát thread chạy trên CPU nào
 * 
 * Biên dịch: gcc -o set_affinity set_affinity.c -pthread
 * Chạy: ./set_affinity
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define NUM_THREADS 4

typedef struct {
    int thread_id;
    int cpu_id;
    long iterations;
} thread_arg_t;

void print_affinity(int thread_id) {
    cpu_set_t cpuset;
    int i;
    
    CPU_ZERO(&cpuset);
    
    if (pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
        perror("pthread_getaffinity_np");
        return;
    }
    
    printf("Thread %d - CPU affinity: ", thread_id);
    for (i = 0; i < CPU_SETSIZE; i++) {
        if (CPU_ISSET(i, &cpuset)) {
            printf("%d ", i);
        }
    }
    printf("\n");
}

void *pinned_thread(void *arg) {
    thread_arg_t *data = (thread_arg_t *)arg;
    cpu_set_t cpuset;
    
    // Pin thread vào CPU cụ thể
    CPU_ZERO(&cpuset);
    CPU_SET(data->cpu_id, &cpuset);
    
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
        perror("pthread_setaffinity_np");
        return NULL;
    }
    
    printf("Thread %d được pin vào CPU %d\n", data->thread_id, data->cpu_id);
    print_affinity(data->thread_id);
    
    // Làm công việc và kiểm tra xem có bị di chuyển không
    int last_cpu = sched_getcpu();
    int migrations = 0;
    long i;
    
    for (i = 0; i < data->iterations; i++) {
        int current_cpu = sched_getcpu();
        if (current_cpu != last_cpu) {
            migrations++;
            printf("Thread %d: Di chuyển từ CPU %d -> %d\n", 
                   data->thread_id, last_cpu, current_cpu);
            last_cpu = current_cpu;
        }
        
        // Làm công việc
        volatile double x = 0;
        int j;
        for (j = 0; j < 1000; j++) {
            x += 1.0 / (j + 1);
        }
    }
    
    printf("Thread %d hoàn thành trên CPU %d (migrations: %d)\n", 
           data->thread_id, sched_getcpu(), migrations);
    
    return NULL;
}

void *unpinned_thread(void *arg) {
    thread_arg_t *data = (thread_arg_t *)arg;
    
    printf("Thread %d KHÔNG pin (có thể chạy trên bất kỳ CPU nào)\n", data->thread_id);
    print_affinity(data->thread_id);
    
    // Làm công việc và theo dõi migrations
    int last_cpu = sched_getcpu();
    int migrations = 0;
    long i;
    
    for (i = 0; i < data->iterations; i++) {
        int current_cpu = sched_getcpu();
        if (current_cpu != last_cpu) {
            migrations++;
            last_cpu = current_cpu;
        }
        
        // Làm công việc
        volatile double x = 0;
        int j;
        for (j = 0; j < 1000; j++) {
            x += 1.0 / (j + 1);
        }
    }
    
    printf("Thread %d hoàn thành (migrations: %d)\n", data->thread_id, migrations);
    
    return NULL;
}

void demo_pinned_threads() {
    pthread_t threads[NUM_THREADS];
    thread_arg_t args[NUM_THREADS];
    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    int i;
    
    printf("\n=== Demo: Pin threads vào CPUs cụ thể ===\n");
    printf("Số CPUs: %d\n\n", num_cpus);
    
    // Tạo threads và pin vào CPUs
    for (i = 0; i < NUM_THREADS; i++) {
        args[i].thread_id = i + 1;
        args[i].cpu_id = i % num_cpus;  // Round-robin trên CPUs
        args[i].iterations = 100000;
        
        if (pthread_create(&threads[i], NULL, pinned_thread, &args[i]) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }
    
    // Đợi threads
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
}

void demo_unpinned_threads() {
    pthread_t threads[NUM_THREADS];
    thread_arg_t args[NUM_THREADS];
    int i;
    
    printf("\n=== Demo: Threads KHÔNG pin ===\n\n");
    
    // Tạo threads không pin
    for (i = 0; i < NUM_THREADS; i++) {
        args[i].thread_id = i + 1;
        args[i].iterations = 100000;
        
        if (pthread_create(&threads[i], NULL, unpinned_thread, &args[i]) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }
    
    // Đợi threads
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
}

void demo_set_mask() {
    pthread_t thread;
    cpu_set_t cpuset;
    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    
    printf("\n=== Demo: Set affinity mask (nhiều CPUs) ===\n\n");
    
    thread_arg_t arg = {.thread_id = 99, .iterations = 50000};
    
    if (pthread_create(&thread, NULL, unpinned_thread, &arg) != 0) {
        perror("pthread_create");
        return;
    }
    
    sleep(1);
    
    // Set affinity cho phép chạy trên CPUs chẵn
    CPU_ZERO(&cpuset);
    int i;
    for (i = 0; i < num_cpus; i += 2) {
        CPU_SET(i, &cpuset);
    }
    
    if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset) == 0) {
        printf("Main: Đã giới hạn thread 99 chỉ chạy trên CPUs chẵn\n");
    }
    
    pthread_join(thread, NULL);
}

void demo_process_affinity() {
    cpu_set_t cpuset;
    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    
    printf("\n=== Demo: Set affinity cho process ===\n\n");
    
    printf("Affinity ban đầu của process:\n");
    if (sched_getaffinity(0, sizeof(cpu_set_t), &cpuset) == 0) {
        printf("CPUs: ");
        int i;
        for (i = 0; i < num_cpus; i++) {
            if (CPU_ISSET(i, &cpuset)) {
                printf("%d ", i);
            }
        }
        printf("\n");
    }
    
    // Giới hạn process chỉ chạy trên CPU 0
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) == 0) {
        printf("\nĐã set process chỉ chạy trên CPU 0\n");
        
        // Kiểm tra lại
        sched_getaffinity(0, sizeof(cpu_set_t), &cpuset);
        printf("CPUs sau khi set: ");
        int i;
        for (i = 0; i < num_cpus; i++) {
            if (CPU_ISSET(i, &cpuset)) {
                printf("%d ", i);
            }
        }
        printf("\n");
    }
}

int main() {
    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    
    printf("=== CPU Affinity Demo ===\n");
    printf("System có %d CPUs\n", num_cpus);
    
    if (num_cpus < 2) {
        printf("Cần ít nhất 2 CPUs để demo hiệu quả\n");
    }
    
    demo_unpinned_threads();
    sleep(1);
    
    demo_pinned_threads();
    sleep(1);
    
    demo_set_mask();
    sleep(1);
    
    demo_process_affinity();
    
    printf("\n=== Tổng kết ===\n");
    printf("CPU Affinity cho phép:\n");
    printf("+ Giảm cache misses (data locality)\n");
    printf("+ Giảm context switches giữa các cores\n");
    printf("+ Cải thiện hiệu năng cho CPU-intensive tasks\n");
    printf("+ Isolate critical threads\n");
    printf("\nNhược điểm:\n");
    printf("- Mất khả năng load balancing tự động\n");
    printf("- Có thể gây imbalance nếu dùng sai\n");
    printf("- Cần hiểu rõ hardware topology\n");
    
    printf("\n=== Công cụ ===\n");
    printf("taskset -c 0,2 ./program  - Chạy trên CPUs 0 và 2\n");
    printf("numactl --cpubind=0 ./program - Bind vào NUMA node 0\n");
    printf("htop (F5)  - Xem thread trên CPU nào\n");
    
    return 0;
}
