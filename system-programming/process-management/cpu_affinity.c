/*
 * cpu_affinity.c - CPU affinity cho tiến trình
 * 
 * CPU affinity xác định tiến trình được chạy trên CPU nào
 * Hữu ích cho tối ưu hiệu suất và cache locality
 * 
 * Biên dịch: gcc -o cpu_affinity cpu_affinity.c -pthread
 * Chạy: ./cpu_affinity
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

void print_cpu_info() {
    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    printf("Số CPU có sẵn: %d\n", num_cpus);
    printf("Số CPU được cấu hình: %ld\n", sysconf(_SC_NPROCESSORS_CONF));
}

void print_affinity(pid_t pid) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    
    if (sched_getaffinity(pid, sizeof(mask), &mask) == -1) {
        perror("sched_getaffinity");
        return;
    }
    
    printf("CPU affinity cho PID %d: ", pid);
    
    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    int count = 0;
    
    for (int i = 0; i < num_cpus; i++) {
        if (CPU_ISSET(i, &mask)) {
            printf("%d ", i);
            count++;
        }
    }
    
    printf("(tổng: %d CPU)\n", count);
}

void example1_get_affinity() {
    printf("\n=== Ví dụ 1: Lấy CPU affinity hiện tại ===\n");
    
    print_cpu_info();
    printf("\n");
    
    pid_t pid = getpid();
    print_affinity(pid);
}

void example2_set_affinity() {
    printf("\n=== Ví dụ 2: Đặt CPU affinity ===\n");
    
    pid_t pid = getpid();
    
    printf("Affinity trước khi thay đổi:\n");
    print_affinity(pid);
    
    // Đặt affinity chỉ cho CPU 0
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    
    printf("\nĐặt affinity chỉ cho CPU 0...\n");
    if (sched_setaffinity(pid, sizeof(mask), &mask) == -1) {
        perror("sched_setaffinity");
        return;
    }
    
    printf("Affinity sau khi thay đổi:\n");
    print_affinity(pid);
    
    // Đặt lại cho nhiều CPU
    printf("\nĐặt affinity cho CPU 0 và 1...\n");
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    CPU_SET(1, &mask);
    
    if (sched_setaffinity(pid, sizeof(mask), &mask) == -1) {
        perror("sched_setaffinity");
        return;
    }
    
    print_affinity(pid);
}

void cpu_intensive_work(int seconds, int cpu_id) {
    printf("[CPU%d] Bắt đầu công việc trong %d giây...\n", cpu_id, seconds);
    
    time_t start = time(NULL);
    volatile long long counter = 0;
    
    while (time(NULL) - start < seconds) {
        counter++;
        
        // Kiểm tra CPU đang chạy
        if (counter % 100000000 == 0) {
            int current_cpu = sched_getcpu();
            printf("[CPU%d] Đang chạy trên CPU %d\n", cpu_id, current_cpu);
        }
    }
    
    printf("[CPU%d] Hoàn thành! Counter = %lld\n", cpu_id, counter);
}

void example3_fork_with_affinity() {
    printf("\n=== Ví dụ 3: Fork với CPU affinity khác nhau ===\n");
    
    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    
    if (num_cpus < 2) {
        printf("Cần ít nhất 2 CPU để chạy ví dụ này\n");
        return;
    }
    
    for (int i = 0; i < 2; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork");
            continue;
        } else if (pid == 0) {
            // Tiến trình con
            cpu_set_t mask;
            CPU_ZERO(&mask);
            CPU_SET(i % num_cpus, &mask);
            
            printf("\n[CON%d] PID=%d, đặt affinity cho CPU %d\n", 
                   i, getpid(), i % num_cpus);
            
            if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
                perror("sched_setaffinity");
                exit(1);
            }
            
            print_affinity(getpid());
            cpu_intensive_work(3, i);
            exit(0);
        }
    }
    
    // Cha đợi
    printf("\n[CHA] Đợi các tiến trình con...\n");
    wait(NULL);
    wait(NULL);
    printf("\n[CHA] Hoàn thành!\n");
}

void* thread_function(void* arg) {
    int thread_id = *(int*)arg;
    int cpu = sched_getcpu();
    
    printf("[THREAD%d] Bắt đầu trên CPU %d\n", thread_id, cpu);
    
    // Làm việc
    volatile long long counter = 0;
    for (int i = 0; i < 100000000; i++) {
        counter++;
    }
    
    cpu = sched_getcpu();
    printf("[THREAD%d] Kết thúc trên CPU %d (counter=%lld)\n", 
           thread_id, cpu, counter);
    
    return NULL;
}

void example4_pthread_affinity() {
    printf("\n=== Ví dụ 4: Thread affinity với pthread ===\n");
    
    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    printf("Số CPU: %d\n\n", num_cpus);
    
    pthread_t threads[4];
    int thread_ids[4];
    
    for (int i = 0; i < 4; i++) {
        thread_ids[i] = i;
        
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        
        // Đặt CPU affinity cho thread
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i % num_cpus, &cpuset);
        
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);
        
        printf("Tạo thread %d với affinity CPU %d\n", i, i % num_cpus);
        
        if (pthread_create(&threads[i], &attr, thread_function, &thread_ids[i]) != 0) {
            perror("pthread_create");
            continue;
        }
        
        pthread_attr_destroy(&attr);
    }
    
    // Đợi threads
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\nTất cả threads đã hoàn thành\n");
}

void example5_inherit_affinity() {
    printf("\n=== Ví dụ 5: Kế thừa CPU affinity ===\n");
    
    pid_t pid = getpid();
    
    printf("Cha - Affinity ban đầu:\n");
    print_affinity(pid);
    
    // Đặt affinity cho cha
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    
    printf("\nĐặt affinity của cha chỉ cho CPU 0\n");
    sched_setaffinity(pid, sizeof(mask), &mask);
    print_affinity(pid);
    
    // Fork
    pid_t child = fork();
    
    if (child < 0) {
        perror("fork");
        return;
    } else if (child == 0) {
        // Con
        printf("\nCon - Affinity (kế thừa từ cha):\n");
        print_affinity(getpid());
        
        printf("\nCon thay đổi affinity của mình sang CPU 1\n");
        CPU_ZERO(&mask);
        CPU_SET(1, &mask);
        sched_setaffinity(0, sizeof(mask), &mask);
        print_affinity(getpid());
        
        exit(0);
    } else {
        wait(NULL);
        
        printf("\nCha - Affinity không thay đổi:\n");
        print_affinity(pid);
    }
}

void print_menu() {
    printf("\n=== MENU: CPU Affinity ===\n");
    printf("1. Lấy CPU affinity hiện tại\n");
    printf("2. Đặt CPU affinity\n");
    printf("3. Fork với CPU affinity khác nhau\n");
    printf("4. Thread affinity với pthread\n");
    printf("5. Kế thừa CPU affinity\n");
    printf("0. Thoát\n");
    printf("Chọn: ");
}

int main() {
    printf("CPU AFFINITY DEMO\n");
    printf("=================\n");
    
    int choice;
    
    while (1) {
        print_menu();
        
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            continue;
        }
        
        switch (choice) {
            case 1:
                example1_get_affinity();
                break;
            case 2:
                example2_set_affinity();
                break;
            case 3:
                example3_fork_with_affinity();
                break;
            case 4:
                example4_pthread_affinity();
                break;
            case 5:
                example5_inherit_affinity();
                break;
            case 0:
                printf("\nTạm biệt!\n");
                exit(0);
            default:
                printf("\nLựa chọn không hợp lệ!\n");
        }
        
        printf("\nNhấn Enter để tiếp tục...");
        while (getchar() != '\n');
        getchar();
    }
    
    return 0;
}
