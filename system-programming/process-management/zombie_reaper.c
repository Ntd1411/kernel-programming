/*
 * zombie_reaper.c - Xử lý zombie process
 * 
 * Zombie process là tiến trình con đã kết thúc nhưng chưa được reap
 * 
 * Biên dịch: gcc -o zombie_reaper zombie_reaper.c
 * Chạy: ./zombie_reaper
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

void example1_create_zombie() {
    printf("\n=== Ví dụ 1: Tạo zombie process ===\n");
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Con kết thúc ngay
        printf("[CON] PID=%d, kết thúc ngay\n", getpid());
        exit(0);
    } else {
        // Cha không gọi wait() - tạo zombie
        printf("[CHA] Con PID=%d đã tạo\n", pid);
        printf("[CHA] Cha không gọi wait() - con sẽ thành zombie\n");
        printf("[CHA] Kiểm tra zombie trong 5 giây...\n\n");
        
        sleep(2);
        
        // Chạy lệnh ps để kiểm tra zombie
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "ps aux | grep %d | grep -v grep", pid);
        printf("[CHA] Chạy lệnh: %s\n", cmd);
        system(cmd);
        
        printf("\n[CHA] Chú ý: Cột STAT có chữ 'Z' = zombie\n");
        
        sleep(3);
        
        printf("\n[CHA] Giờ gọi wait() để reap zombie\n");
        wait(NULL);
        printf("[CHA] Đã reap zombie process\n");
        
        sleep(1);
        printf("\n[CHA] Kiểm tra lại sau khi reap:\n");
        system(cmd);
        printf("[CHA] Process không còn tồn tại (đã reap)\n");
    }
}

void example2_sigchld_handler() {
    printf("\n=== Ví dụ 2: Dùng SIGCHLD để tự động reap ===\n");
    
    // Handler cho SIGCHLD
    struct sigaction sa;
    sa.sa_handler = SIG_IGN; // Ignore SIGCHLD = auto-reap
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_NOCLDWAIT; // Không tạo zombie
    
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    
    printf("[CHA] Đã đăng ký handler SIGCHLD\n");
    printf("[CHA] Các tiến trình con sẽ tự động được reap\n\n");
    
    // Tạo nhiều tiến trình con
    for (int i = 0; i < 5; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork");
            continue;
        } else if (pid == 0) {
            printf("  [CON %d] PID=%d, sleep %d giây\n", i + 1, getpid(), i + 1);
            sleep(i + 1);
            exit(i);
        }
        
        printf("[CHA] Tạo con #%d, PID=%d\n", i + 1, pid);
    }
    
    printf("\n[CHA] Đợi 10 giây để các con kết thúc...\n");
    sleep(10);
    printf("[CHA] Không có zombie vì đã dùng SA_NOCLDWAIT\n");
}

volatile sig_atomic_t child_died = 0;

void sigchld_reaper(int signum) {
    child_died = 1;
}

void example3_waitpid_wnohang() {
    printf("\n=== Ví dụ 3: waitpid() với WNOHANG ===\n");
    
    signal(SIGCHLD, sigchld_reaper);
    
    printf("[CHA] PID=%d\n", getpid());
    
    // Tạo 3 tiến trình con
    for (int i = 0; i < 3; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork");
            continue;
        } else if (pid == 0) {
            printf("  [CON %d] PID=%d, sleep %d giây\n", 
                   i + 1, getpid(), (i + 1) * 2);
            sleep((i + 1) * 2);
            printf("  [CON %d] Kết thúc\n", i + 1);
            exit(i);
        }
    }
    
    // Cha làm việc khác và reap khi cần
    int children_left = 3;
    
    while (children_left > 0) {
        printf("[CHA] Đang làm việc khác...\n");
        sleep(1);
        
        if (child_died) {
            child_died = 0;
            
            // Reap tất cả con đã chết
            pid_t pid;
            int status;
            
            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                if (WIFEXITED(status)) {
                    printf("[CHA] Reap con PID=%d, exit=%d\n",
                           pid, WEXITSTATUS(status));
                    children_left--;
                }
            }
        }
    }
    
    printf("[CHA] Tất cả con đã được reap\n");
}

void example4_wait_specific_child() {
    printf("\n=== Ví dụ 4: waitpid() cho tiến trình cụ thể ===\n");
    
    pid_t pids[3];
    
    // Tạo 3 con
    for (int i = 0; i < 3; i++) {
        pids[i] = fork();
        
        if (pids[i] < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pids[i] == 0) {
            printf("  [CON %d] PID=%d\n", i + 1, getpid());
            sleep((3 - i) * 2); // Con đầu sleep lâu nhất
            exit(i);
        }
    }
    
    printf("[CHA] Đã tạo 3 con: %d, %d, %d\n", pids[0], pids[1], pids[2]);
    printf("[CHA] Đợi con giữa (PID=%d) trước\n", pids[1]);
    
    int status;
    pid_t result = waitpid(pids[1], &status, 0);
    
    if (result > 0 && WIFEXITED(status)) {
        printf("[CHA] Con PID=%d đã kết thúc, exit=%d\n",
               result, WEXITSTATUS(status));
    }
    
    printf("[CHA] Giờ đợi các con còn lại\n");
    
    while (wait(NULL) > 0) {
        printf("[CHA] Reap thêm một con\n");
    }
    
    printf("[CHA] Tất cả con đã kết thúc\n");
}

void example5_double_fork() {
    printf("\n=== Ví dụ 5: Double fork để tránh zombie ===\n");
    
    pid_t pid1 = fork();
    
    if (pid1 < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid1 == 0) {
        // Con thứ nhất
        printf("[CON1] PID=%d, PPID=%d\n", getpid(), getppid());
        
        pid_t pid2 = fork();
        
        if (pid2 < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid2 == 0) {
            // Cháu (con thứ hai)
            printf("[CHAU] PID=%d, PPID=%d\n", getpid(), getppid());
            printf("[CHAU] Làm công việc dài hạn (10 giây)\n");
            sleep(10);
            printf("[CHAU] Hoàn thành, PPID giờ là=%d (init)\n", getppid());
            exit(0);
        }
        
        printf("[CON1] Tạo cháu PID=%d, kết thúc ngay\n", pid2);
        exit(0);
    } else {
        // Cha
        printf("[CHA] PID=%d, tạo con PID=%d\n", getpid(), pid1);
        
        // Đợi con thứ nhất
        wait(NULL);
        printf("[CHA] Con thứ nhất đã kết thúc\n");
        printf("[CHA] Cháu (PID con của con) giờ được init adopt\n");
        printf("[CHA] Không có zombie!\n");
        
        sleep(2);
        printf("[CHA] Cha kết thúc trước cháu - không vấn đề\n");
    }
}

int main(int argc, char *argv[]) {
    printf("=== ZOMBIE REAPER EXAMPLES ===\n");
    printf("PID của chương trình: %d\n", getpid());
    
    if (argc > 1) {
        int example = atoi(argv[1]);
        
        switch (example) {
            case 1:
                example1_create_zombie();
                break;
            case 2:
                example2_sigchld_handler();
                break;
            case 3:
                example3_waitpid_wnohang();
                break;
            case 4:
                example4_wait_specific_child();
                break;
            case 5:
                example5_double_fork();
                break;
            default:
                printf("Ví dụ không hợp lệ\n");
        }
    } else {
        printf("\nChọn ví dụ:\n");
        printf("  1 - Tạo và reap zombie\n");
        printf("  2 - SIGCHLD auto-reap\n");
        printf("  3 - waitpid() với WNOHANG\n");
        printf("  4 - waitpid() cho tiến trình cụ thể\n");
        printf("  5 - Double fork\n");
        printf("\nSử dụng: %s [1-5]\n", argv[0]);
    }
    
    return 0;
}
