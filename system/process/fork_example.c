/*
 * fork_example.c - Ví dụ về fork() để tạo tiến trình con
 * 
 * fork() tạo một bản sao của tiến trình hiện tại
 * 
 * Biên dịch: gcc -o fork_example fork_example.c
 * Chạy: ./fork_example
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void example1_basic_fork() {
    printf("\n=== Ví dụ 1: Fork cơ bản ===\n");
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Tiến trình con
        printf("[CON] PID=%d, PPID=%d\n", getpid(), getppid());
        printf("[CON] Đây là tiến trình con\n");
        sleep(2);
        printf("[CON] Kết thúc\n");
        exit(0);
    } else {
        // Tiến trình cha
        printf("[CHA] PID=%d, Con PID=%d\n", getpid(), pid);
        printf("[CHA] Đây là tiến trình cha\n");
        wait(NULL);
        printf("[CHA] Con đã kết thúc\n");
    }
}

void example2_multiple_forks() {
    printf("\n=== Ví dụ 2: Nhiều fork ===\n");
    
    for (int i = 0; i < 3; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Tiến trình con
            printf("[CON %d] PID=%d, PPID=%d\n", i + 1, getpid(), getppid());
            sleep(1);
            exit(i);
        }
    }
    
    // Cha đợi tất cả con
    printf("[CHA] Đợi các tiến trình con...\n");
    
    for (int i = 0; i < 3; i++) {
        int status;
        pid_t pid = wait(&status);
        
        if (WIFEXITED(status)) {
            printf("[CHA] Con PID=%d kết thúc với exit code=%d\n",
                   pid, WEXITSTATUS(status));
        }
    }
}

void example3_fork_tree() {
    printf("\n=== Ví dụ 3: Fork tree ===\n");
    printf("Tạo cây tiến trình: Cha -> 2 Con -> Mỗi con có 2 cháu\n\n");
    
    pid_t pid1 = fork();
    
    if (pid1 < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid1 == 0) {
        // Con thứ nhất
        printf("[CON1] PID=%d, PPID=%d\n", getpid(), getppid());
        
        // Con1 tạo 2 cháu
        for (int i = 0; i < 2; i++) {
            pid_t grandchild = fork();
            if (grandchild == 0) {
                printf("  [CHAU1.%d] PID=%d, PPID=%d\n", 
                       i + 1, getpid(), getppid());
                sleep(1);
                exit(0);
            }
        }
        
        wait(NULL);
        wait(NULL);
        exit(0);
    }
    
    pid_t pid2 = fork();
    
    if (pid2 < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid2 == 0) {
        // Con thứ hai
        printf("[CON2] PID=%d, PPID=%d\n", getpid(), getppid());
        
        // Con2 tạo 2 cháu
        for (int i = 0; i < 2; i++) {
            pid_t grandchild = fork();
            if (grandchild == 0) {
                printf("  [CHAU2.%d] PID=%d, PPID=%d\n", 
                       i + 1, getpid(), getppid());
                sleep(1);
                exit(0);
            }
        }
        
        wait(NULL);
        wait(NULL);
        exit(0);
    }
    
    // Cha đợi 2 con
    printf("[CHA] PID=%d, đợi các con...\n", getpid());
    wait(NULL);
    wait(NULL);
    printf("[CHA] Tất cả con đã kết thúc\n");
}

void example4_shared_data() {
    printf("\n=== Ví dụ 4: Dữ liệu không chia sẻ sau fork ===\n");
    
    int value = 100;
    printf("Trước fork: value=%d\n", value);
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Con thay đổi giá trị
        value = 200;
        printf("[CON] Thay đổi value=%d\n", value);
        exit(0);
    } else {
        // Cha
        wait(NULL);
        printf("[CHA] Sau khi con kết thúc: value=%d\n", value);
        printf("[CHA] Giá trị không thay đổi vì mỗi tiến trình có bộ nhớ riêng\n");
    }
}

int main() {
    printf("=== FORK EXAMPLES ===\n");
    printf("PID: %d\n", getpid());
    
    example1_basic_fork();
    sleep(1);
    
    example2_multiple_forks();
    sleep(1);
    
    example3_fork_tree();
    sleep(1);
    
    example4_shared_data();
    
    printf("\n=== HOÀN THÀNH ===\n");
    printf("\nTóm tắt fork():\n");
    printf("  fork()       - tạo tiến trình con giống hệt cha\n");
    printf("  getpid()     - lấy PID của tiến trình hiện tại\n");
    printf("  getppid()    - lấy PID của tiến trình cha\n");
    printf("  wait()       - đợi tiến trình con kết thúc\n");
    printf("  exit()       - kết thúc tiến trình con\n");
    printf("  Lưu ý: Con và cha có bộ nhớ riêng biệt\n");
    
    return 0;
}
