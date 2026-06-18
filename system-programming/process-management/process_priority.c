/*
 * process_priority.c - Quản lý priority và scheduling của tiến trình
 * 
 * Các khái niệm:
 * - Nice value: -20 (highest priority) đến 19 (lowest priority)
 * - Real-time scheduling: SCHED_FIFO, SCHED_RR, SCHED_DEADLINE
 * - Normal scheduling: SCHED_OTHER, SCHED_BATCH, SCHED_IDLE
 * 
 * Biên dịch: gcc -o process_priority process_priority.c
 * Chạy: sudo ./process_priority (cần sudo cho real-time scheduling)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <sched.h>
#include <errno.h>
#include <string.h>

#ifndef SCHED_BATCH
#define SCHED_BATCH 3
#endif

#ifndef SCHED_IDLE
#define SCHED_IDLE 5
#endif

void print_priority_info(pid_t pid) {
    int nice_value = getpriority(PRIO_PROCESS, pid);
    
    printf("PID: %d\n", pid);
    printf("Nice value: %d\n", nice_value);
    
    int policy = sched_getscheduler(pid);
    const char *policy_name;
    
    switch (policy) {
        case SCHED_OTHER: policy_name = "SCHED_OTHER (normal)"; break;
        case SCHED_FIFO: policy_name = "SCHED_FIFO (real-time)"; break;
        case SCHED_RR: policy_name = "SCHED_RR (real-time)"; break;
        case SCHED_BATCH: policy_name = "SCHED_BATCH"; break;
        case SCHED_IDLE: policy_name = "SCHED_IDLE"; break;
        default: policy_name = "Unknown"; break;
    }
    
    printf("Scheduling policy: %s\n", policy_name);
    
    struct sched_param param;
    if (sched_getparam(pid, &param) == 0) {
        printf("Priority: %d\n", param.sched_priority);
    }
    
    printf("Min priority for policy: %d\n", sched_get_priority_min(policy));
    printf("Max priority for policy: %d\n\n", sched_get_priority_max(policy));
}

void example1_nice_value() {
    printf("\n=== Ví dụ 1: Thay đổi nice value ===\n");
    
    pid_t pid = getpid();
    
    printf("Trước khi thay đổi:\n");
    print_priority_info(pid);
    
    // Tăng nice value (giảm priority)
    printf("Tăng nice value lên 10...\n");
    if (nice(10) == -1 && errno != 0) {
        perror("nice");
    } else {
        printf("\nSau khi thay đổi:\n");
        print_priority_info(pid);
    }
    
    // Reset về 0 (cần quyền root)
    printf("Thử đặt nice về 0 (có thể cần sudo)...\n");
    if (setpriority(PRIO_PROCESS, 0, 0) == -1) {
        perror("setpriority");
        printf("Lưu ý: Cần quyền root để giảm nice value\n");
    } else {
        printf("\nSau khi reset:\n");
        print_priority_info(pid);
    }
}

void example2_getpriority_range() {
    printf("\n=== Ví dụ 2: Lấy priority của nhiều tiến trình ===\n");
    
    // Priority của tiến trình hiện tại
    int prio = getpriority(PRIO_PROCESS, 0);
    printf("Priority của tiến trình hiện tại: %d\n", prio);
    
    // Priority của process group
    pid_t pgid = getpgrp();
    prio = getpriority(PRIO_PGRP, pgid);
    printf("Priority của process group %d: %d\n", pgid, prio);
    
    // Priority của user
    uid_t uid = getuid();
    prio = getpriority(PRIO_USER, uid);
    printf("Priority của user %d: %d\n", uid, prio);
}

void cpu_intensive_work(int seconds) {
    printf("Bắt đầu công việc tốn CPU trong %d giây...\n", seconds);
    time_t start = time(NULL);
    volatile long long counter = 0;
    
    while (time(NULL) - start < seconds) {
        counter++;
    }
    
    printf("Hoàn thành! Counter = %lld\n", counter);
}

void example3_priority_comparison() {
    printf("\n=== Ví dụ 3: So sánh hiệu suất với nice value khác nhau ===\n");
    
    pid_t pid1 = fork();
    
    if (pid1 < 0) {
        perror("fork");
        return;
    } else if (pid1 == 0) {
        // Con 1: Priority cao (nice = -10, cần root)
        printf("\n[CON1] PID=%d, đặt nice=-10\n", getpid());
        if (setpriority(PRIO_PROCESS, 0, -10) == -1) {
            printf("[CON1] Không thể đặt nice=-10, dùng nice=0\n");
        }
        print_priority_info(getpid());
        cpu_intensive_work(3);
        exit(0);
    }
    
    pid_t pid2 = fork();
    
    if (pid2 < 0) {
        perror("fork");
        return;
    } else if (pid2 == 0) {
        // Con 2: Priority thấp (nice = 19)
        printf("\n[CON2] PID=%d, đặt nice=19\n", getpid());
        setpriority(PRIO_PROCESS, 0, 19);
        print_priority_info(getpid());
        cpu_intensive_work(3);
        exit(0);
    }
    
    // Cha đợi
    printf("\n[CHA] Đợi 2 tiến trình con hoàn thành...\n");
    wait(NULL);
    wait(NULL);
    printf("\n[CHA] Hoàn thành!\n");
}

void example4_realtime_scheduling() {
    printf("\n=== Ví dụ 4: Real-time scheduling (cần sudo) ===\n");
    
    struct sched_param param;
    
    printf("Thử đặt SCHED_FIFO với priority 50...\n");
    param.sched_priority = 50;
    
    if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
        perror("sched_setscheduler");
        printf("\nLỗi: Cần quyền root để sử dụng real-time scheduling\n");
        printf("Chạy lại với: sudo ./process_priority\n");
        return;
    }
    
    printf("\nThành công! Thông tin sau khi đặt real-time:\n");
    print_priority_info(getpid());
    
    // Thử SCHED_RR
    printf("\nThử chuyển sang SCHED_RR...\n");
    param.sched_priority = 30;
    if (sched_setscheduler(0, SCHED_RR, &param) == 0) {
        printf("Thành công!\n");
        print_priority_info(getpid());
    }
    
    // Reset về SCHED_OTHER
    printf("\nReset về SCHED_OTHER...\n");
    param.sched_priority = 0;
    sched_setscheduler(0, SCHED_OTHER, &param);
    print_priority_info(getpid());
}

void example5_scheduling_policies() {
    printf("\n=== Ví dụ 5: Các scheduling policy khác nhau ===\n");
    
    printf("Thông tin về scheduling policies:\n\n");
    
    int policies[] = {SCHED_OTHER, SCHED_FIFO, SCHED_RR, SCHED_BATCH, SCHED_IDLE};
    const char *names[] = {"SCHED_OTHER", "SCHED_FIFO", "SCHED_RR", "SCHED_BATCH", "SCHED_IDLE"};
    
    for (int i = 0; i < 5; i++) {
        int min_prio = sched_get_priority_min(policies[i]);
        int max_prio = sched_get_priority_max(policies[i]);
        
        printf("%s:\n", names[i]);
        printf("  Min priority: %d\n", min_prio);
        printf("  Max priority: %d\n", max_prio);
        printf("  Range: %d\n\n", max_prio - min_prio);
    }
}

void print_menu() {
    printf("\n=== MENU: Process Priority và Scheduling ===\n");
    printf("1. Nice value\n");
    printf("2. Lấy priority của nhiều đối tượng\n");
    printf("3. So sánh hiệu suất với nice value khác nhau\n");
    printf("4. Real-time scheduling (cần sudo)\n");
    printf("5. Thông tin về scheduling policies\n");
    printf("0. Thoát\n");
    printf("Chọn: ");
}

int main() {
    printf("=== PROCESS PRIORITY AND SCHEDULING EXAMPLES ===\n");
    printf("PID: %d\n", getpid());
    
    example1_nice_value();
    sleep(1);
    
    example2_getpriority_range();
    sleep(1);
    
    example3_priority_comparison();
    sleep(1);
    
    example4_realtime_scheduling();
    sleep(1);
    
    example5_scheduling_policies();
    
    printf("\n=== HOÀN THÀNH ===\n");
    printf("\nTóm tắt process priority:\n");
    printf("  nice()           - thay đổi nice value (tương đối)\n");
    printf("  setpriority()    - đặt nice value (tuyệt đối)\n");
    printf("  getpriority()    - lấy nice value\n");
    printf("  sched_setscheduler() - đặt scheduling policy\n");
    printf("  sched_getscheduler() - lấy scheduling policy\n");
    printf("  SCHED_OTHER      - scheduling mặc định\n");
    printf("  SCHED_FIFO/RR    - real-time (cần sudo)\n");
    
    return 0;
}
