/*
 * cpu_info.c - Hiển thị thông tin CPU
 * 
 * Lấy thông tin về CPU cores, threads, cache, NUMA nodes
 * 
 * Biên dịch: gcc -o cpu_info cpu_info.c -pthread
 * Chạy: ./cpu_info
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <errno.h>

void print_cpu_info() {
    long num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    long num_configured = sysconf(_SC_NPROCESSORS_CONF);
    
    printf("=== CPU Information ===\n");
    printf("CPUs online: %ld\n", num_cpus);
    printf("CPUs configured: %ld\n", num_configured);
    
    // Cache information
    long l1_cache = sysconf(_SC_LEVEL1_DCACHE_SIZE);
    long l2_cache = sysconf(_SC_LEVEL2_CACHE_SIZE);
    long l3_cache = sysconf(_SC_LEVEL3_CACHE_SIZE);
    
    if (l1_cache > 0) printf("L1 Data Cache: %ld KB\n", l1_cache / 1024);
    if (l2_cache > 0) printf("L2 Cache: %ld KB\n", l2_cache / 1024);
    if (l3_cache > 0) printf("L3 Cache: %ld KB\n", l3_cache / 1024);
    
    // Page size
    long page_size = sysconf(_SC_PAGESIZE);
    printf("Page size: %ld bytes\n", page_size);
    
    // Physical memory
    long phys_pages = sysconf(_SC_PHYS_PAGES);
    long avail_pages = sysconf(_SC_AVPHYS_PAGES);
    printf("Total RAM: %.2f GB\n", (phys_pages * page_size) / (1024.0 * 1024.0 * 1024.0));
    printf("Available RAM: %.2f GB\n", (avail_pages * page_size) / (1024.0 * 1024.0 * 1024.0));
}

void print_current_cpu() {
    int cpu = sched_getcpu();
    if (cpu == -1) {
        perror("sched_getcpu");
    } else {
        printf("Thread đang chạy trên CPU: %d\n", cpu);
    }
}

void print_cpu_affinity() {
    cpu_set_t cpuset;
    int i;
    
    CPU_ZERO(&cpuset);
    
    if (sched_getaffinity(0, sizeof(cpu_set_t), &cpuset) == -1) {
        perror("sched_getaffinity");
        return;
    }
    
    printf("CPU affinity mask: ");
    for (i = 0; i < CPU_SETSIZE; i++) {
        if (CPU_ISSET(i, &cpuset)) {
            printf("%d ", i);
        }
    }
    printf("\n");
}

void print_scheduling_policy() {
    int policy = sched_getscheduler(0);
    struct sched_param param;
    
    printf("\n=== Scheduling Information ===\n");
    
    switch (policy) {
        case SCHED_OTHER:
            printf("Scheduling policy: SCHED_OTHER (normal)\n");
            break;
        case SCHED_FIFO:
            printf("Scheduling policy: SCHED_FIFO (real-time FIFO)\n");
            break;
        case SCHED_RR:
            printf("Scheduling policy: SCHED_RR (real-time round-robin)\n");
            break;
        case SCHED_BATCH:
            printf("Scheduling policy: SCHED_BATCH (batch)\n");
            break;
        case SCHED_IDLE:
            printf("Scheduling policy: SCHED_IDLE (idle)\n");
            break;
        default:
            printf("Scheduling policy: Unknown (%d)\n", policy);
    }
    
    if (sched_getparam(0, &param) == 0) {
        printf("Priority: %d\n", param.sched_priority);
    }
    
    // Priority range
    int min_prio = sched_get_priority_min(policy);
    int max_prio = sched_get_priority_max(policy);
    printf("Priority range: %d - %d\n", min_prio, max_prio);
}

void *worker_thread(void *arg) {
    int id = *(int *)arg;
    
    printf("\nThread %d:\n", id);
    print_current_cpu();
    print_cpu_affinity();
    
    // Làm công việc và kiểm tra CPU migration
    int i;
    int last_cpu = sched_getcpu();
    int migrations = 0;
    
    for (i = 0; i < 1000000; i++) {
        int current_cpu = sched_getcpu();
        if (current_cpu != last_cpu && current_cpu != -1) {
            migrations++;
            last_cpu = current_cpu;
        }
    }
    
    printf("Thread %d: Số lần di chuyển giữa CPUs: %d\n", id, migrations);
    
    return NULL;
}

void demo_thread_migration() {
    pthread_t threads[4];
    int thread_ids[4];
    int i;
    
    printf("\n=== Demo Thread Migration ===\n");
    printf("Tạo 4 threads và quan sát CPU migration\n");
    
    for (i = 0; i < 4; i++) {
        thread_ids[i] = i + 1;
        pthread_create(&threads[i], NULL, worker_thread, &thread_ids[i]);
    }
    
    for (i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }
}

void print_proc_cpuinfo() {
    printf("\n=== Đọc /proc/cpuinfo ===\n");
    
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (fp == NULL) {
        perror("fopen /proc/cpuinfo");
        return;
    }
    
    char line[256];
    int cpu_count = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "processor", 9) == 0) {
            cpu_count++;
        } else if (strncmp(line, "model name", 10) == 0) {
            if (cpu_count == 1) {
                printf("%s", line);
            }
        } else if (strncmp(line, "cpu MHz", 7) == 0) {
            if (cpu_count == 1) {
                printf("%s", line);
            }
        } else if (strncmp(line, "cache size", 10) == 0) {
            if (cpu_count == 1) {
                printf("%s", line);
            }
        }
    }
    
    printf("Total processors: %d\n", cpu_count);
    
    fclose(fp);
}

int main() {
    printf("=== CPU Information Tool ===\n\n");
    
    print_cpu_info();
    
    printf("\n=== Current Process ===\n");
    printf("PID: %d\n", getpid());
    print_current_cpu();
    print_cpu_affinity();
    
    print_scheduling_policy();
    
    print_proc_cpuinfo();
    
    demo_thread_migration();
    
    printf("\n=== Lệnh hữu ích ===\n");
    printf("lscpu           - Xem thông tin CPU chi tiết\n");
    printf("nproc           - Số lượng CPUs\n");
    printf("cat /proc/cpuinfo - Thông tin CPU từ kernel\n");
    printf("numactl -H      - Thông tin NUMA topology\n");
    printf("lstopo          - Visualize hardware topology (cần cài hwloc)\n");
    printf("taskset -c 0-3 ./program - Chạy program trên CPUs 0-3\n");
    printf("htop            - Monitor CPU usage per core\n");
    
    return 0;
}
