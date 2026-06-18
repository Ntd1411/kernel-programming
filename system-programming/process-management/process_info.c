/*
 * process_info.c - Lấy thông tin về tiến trình
 * 
 * Các nguồn thông tin:
 * - /proc/[pid]/ filesystem
 * - getrusage() - thống kê tài nguyên
 * - times() - CPU time
 * 
 * Biên dịch: gcc -o process_info process_info.c
 * Chạy: ./process_info
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <errno.h>

void print_separator() {
    printf("================================================\n");
}

void example1_proc_status() {
    printf("\n=== Ví dụ 1: Đọc /proc/[pid]/status ===\n");
    
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", getpid());
    
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        perror("fopen");
        return;
    }
    
    printf("\nThông tin từ %s:\n", path);
    print_separator();
    
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        // Chỉ hiển thị các trường quan trọng
        if (strncmp(line, "Name:", 5) == 0 ||
            strncmp(line, "State:", 6) == 0 ||
            strncmp(line, "Pid:", 4) == 0 ||
            strncmp(line, "PPid:", 5) == 0 ||
            strncmp(line, "Uid:", 4) == 0 ||
            strncmp(line, "Gid:", 4) == 0 ||
            strncmp(line, "Threads:", 8) == 0 ||
            strncmp(line, "VmSize:", 7) == 0 ||
            strncmp(line, "VmRSS:", 6) == 0 ||
            strncmp(line, "VmData:", 7) == 0 ||
            strncmp(line, "VmStk:", 6) == 0) {
            printf("%s", line);
        }
    }
    
    fclose(fp);
}

void example2_proc_stat() {
    printf("\n=== Ví dụ 2: Đọc /proc/[pid]/stat ===\n");
    
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", getpid());
    
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        perror("fopen");
        return;
    }
    
    int pid, ppid, pgrp, session, tty_nr;
    char comm[256], state;
    unsigned long utime, stime;
    long priority, nice_val, num_threads;
    unsigned long long starttime;
    
    fscanf(fp, "%d %s %c %d %d %d %d %*d %*u %*u %*u %*u %*u %lu %lu "
           "%*d %*d %ld %ld %ld %*d %llu",
           &pid, comm, &state, &ppid, &pgrp, &session, &tty_nr,
           &utime, &stime, &priority, &nice_val, &num_threads, &starttime);
    
    fclose(fp);
    
    printf("\nThông tin từ %s:\n", path);
    print_separator();
    printf("PID:          %d\n", pid);
    printf("Command:      %s\n", comm);
    printf("State:        %c\n", state);
    printf("PPID:         %d\n", ppid);
    printf("PGRP:         %d\n", pgrp);
    printf("Session:      %d\n", session);
    printf("TTY:          %d\n", tty_nr);
    printf("User time:    %lu ticks\n", utime);
    printf("System time:  %lu ticks\n", stime);
    printf("Priority:     %ld\n", priority);
    printf("Nice:         %ld\n", nice_val);
    printf("Threads:      %ld\n", num_threads);
}

void example3_proc_cmdline() {
    printf("\n=== Ví dụ 3: Đọc /proc/[pid]/cmdline ===\n");
    
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/cmdline", getpid());
    
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return;
    }
    
    char cmdline[1024];
    ssize_t bytes = read(fd, cmdline, sizeof(cmdline) - 1);
    close(fd);
    
    if (bytes < 0) {
        perror("read");
        return;
    }
    
    printf("\nCommand line arguments:\n");
    print_separator();
    
    // Arguments được phân tách bởi null byte
    for (ssize_t i = 0; i < bytes; i++) {
        if (cmdline[i] == '\0') {
            printf("\n");
        } else {
            printf("%c", cmdline[i]);
        }
    }
    printf("\n");
}

void example4_getrusage() {
    printf("\n=== Ví dụ 4: getrusage() - Thống kê tài nguyên ===\n");
    
    struct rusage usage;
    
    if (getrusage(RUSAGE_SELF, &usage) == -1) {
        perror("getrusage");
        return;
    }
    
    printf("\nThống kê tài nguyên của tiến trình hiện tại:\n");
    print_separator();
    
    printf("User CPU time:        %ld.%06ld giây\n",
           usage.ru_utime.tv_sec, usage.ru_utime.tv_usec);
    printf("System CPU time:      %ld.%06ld giây\n",
           usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
    printf("Max RSS:              %ld KB\n", usage.ru_maxrss);
    printf("Page reclaims:        %ld\n", usage.ru_minflt);
    printf("Page faults:          %ld\n", usage.ru_majflt);
    printf("Block input ops:      %ld\n", usage.ru_inblock);
    printf("Block output ops:     %ld\n", usage.ru_oublock);
    printf("Voluntary switches:   %ld\n", usage.ru_nvcsw);
    printf("Involuntary switches: %ld\n", usage.ru_nivcsw);
}

void cpu_intensive_work(int iterations) {
    volatile long long counter = 0;
    for (int i = 0; i < iterations; i++) {
        counter++;
    }
}

void example5_measure_cpu_time() {
    printf("\n=== Ví dụ 5: Đo CPU time với times() ===\n");
    
    struct tms start, end;
    clock_t start_real, end_real;
    
    // Lấy thời gian bắt đầu
    start_real = times(&start);
    
    printf("\nBắt đầu công việc tốn CPU...\n");
    cpu_intensive_work(100000000);
    
    // Lấy thời gian kết thúc
    end_real = times(&end);
    
    long ticks_per_sec = sysconf(_SC_CLK_TCK);
    
    printf("\nKết quả:\n");
    print_separator();
    printf("Real time:   %.2f giây\n",
           (double)(end_real - start_real) / ticks_per_sec);
    printf("User time:   %.2f giây\n",
           (double)(end.tms_utime - start.tms_utime) / ticks_per_sec);
    printf("System time: %.2f giây\n",
           (double)(end.tms_stime - start.tms_stime) / ticks_per_sec);
}

void example6_child_resource_usage() {
    printf("\n=== Ví dụ 6: Thống kê tài nguyên của tiến trình con ===\n");
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        return;
    } else if (pid == 0) {
        // Con làm việc
        printf("\n[CON] Đang làm việc...\n");
        cpu_intensive_work(50000000);
        printf("[CON] Hoàn thành\n");
        exit(0);
    } else {
        // Cha đợi con
        int status;
        wait(&status);
        
        // Lấy thống kê của con
        struct rusage usage;
        if (getrusage(RUSAGE_CHILDREN, &usage) == -1) {
            perror("getrusage");
            return;
        }
        
        printf("\nThống kê tài nguyên của tiến trình con:\n");
        print_separator();
        printf("User CPU time:   %ld.%06ld giây\n",
               usage.ru_utime.tv_sec, usage.ru_utime.tv_usec);
        printf("System CPU time: %ld.%06ld giây\n",
               usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
        printf("Max RSS:         %ld KB\n", usage.ru_maxrss);
    }
}

void example7_proc_environ() {
    printf("\n=== Ví dụ 7: Đọc /proc/[pid]/environ ===\n");
    
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/environ", getpid());
    
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return;
    }
    
    char buffer[4096];
    ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);
    
    if (bytes < 0) {
        perror("read");
        return;
    }
    
    printf("\nBiến môi trường (5 đầu tiên):\n");
    print_separator();
    
    int count = 0;
    for (ssize_t i = 0; i < bytes && count < 5; i++) {
        if (buffer[i] == '\0') {
            printf("\n");
            count++;
        } else {
            printf("%c", buffer[i]);
        }
    }
    printf("...\n");
}

void print_menu() {
    printf("\n=== MENU: Process Information ===\n");
    printf("1. Đọc /proc/[pid]/status\n");
    printf("2. Đọc /proc/[pid]/stat\n");
    printf("3. Đọc /proc/[pid]/cmdline\n");
    printf("4. getrusage() - Thống kê tài nguyên\n");
    printf("5. Đo CPU time với times()\n");
    printf("6. Thống kê tài nguyên tiến trình con\n");
    printf("7. Đọc /proc/[pid]/environ\n");
    printf("0. Thoát\n");
    printf("Chọn: ");
}

int main() {
    printf("=== PROCESS INFORMATION EXAMPLES ===\n");
    printf("PID: %d\n", getpid());
    
    example1_proc_status();
    sleep(1);
    
    example2_proc_stat();
    sleep(1);
    
    example3_proc_cmdline();
    sleep(1);
    
    example4_getrusage();
    sleep(1);
    
    example5_measure_cpu_time();
    sleep(1);
    
    example6_child_resource_usage();
    sleep(1);
    
    example7_proc_environ();
    
    printf("\n=== HOÀN THÀNH ===\n");
    printf("\nTóm tắt process information:\n");
    printf("  /proc/[pid]/status  - thông tin chi tiết dạng text\n");
    printf("  /proc/[pid]/stat    - thông tin dạng số, dễ parse\n");
    printf("  /proc/[pid]/cmdline - command line arguments\n");
    printf("  /proc/[pid]/environ - biến môi trường\n");
    printf("  getrusage()         - thống kê tài nguyên\n");
    printf("  times()             - CPU time usage\n");
    
    return 0;
}
