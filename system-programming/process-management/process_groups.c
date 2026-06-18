/*
 * process_groups.c - Process groups và sessions
 * 
 * Khái niệm:
 * - Process Group (PGID): Nhóm các tiến trình liên quan
 * - Session (SID): Tập hợp các process groups
 * - Job control: Quản lý foreground/background jobs
 * 
 * Biên dịch: gcc -o process_groups process_groups.c
 * Chạy: ./process_groups
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

void print_ids(const char *label) {
    printf("%s:\n", label);
    printf("  PID  = %d\n", getpid());
    printf("  PPID = %d\n", getppid());
    printf("  PGID = %d\n", getpgrp());
    printf("  SID  = %d\n", getsid(0));
    printf("\n");
}

void example1_basic_ids() {
    printf("\n=== Ví dụ 1: Các ID cơ bản ===\n");
    
    print_ids("Tiến trình cha");
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        return;
    } else if (pid == 0) {
        print_ids("Tiến trình con (mặc định cùng PGID với cha)");
        exit(0);
    } else {
        wait(NULL);
    }
}

void example2_create_process_group() {
    printf("\n=== Ví dụ 2: Tạo process group mới ===\n");
    
    print_ids("Cha");
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        return;
    } else if (pid == 0) {
        // Con tạo process group mới
        printf("Con tạo process group mới...\n");
        
        if (setpgid(0, 0) == -1) {
            perror("setpgid");
            exit(1);
        }
        
        print_ids("Con (process group leader mới)");
        
        sleep(2);
        exit(0);
    } else {
        sleep(1);
        
        printf("Kiểm tra PGID của con từ cha:\n");
        pid_t child_pgid = getpgid(pid);
        printf("  Con PID  = %d\n", pid);
        printf("  Con PGID = %d\n", child_pgid);
        printf("  Cha PGID = %d\n", getpgrp());
        
        wait(NULL);
    }
}

void example3_process_group_family() {
    printf("\n=== Ví dụ 3: Nhóm tiến trình gia đình ===\n");
    
    pid_t leader_pid = fork();
    
    if (leader_pid < 0) {
        perror("fork");
        return;
    } else if (leader_pid == 0) {
        // Leader tạo process group mới
        setpgid(0, 0);
        
        printf("\n[LEADER] PID=%d, PGID=%d\n", getpid(), getpgrp());
        
        // Tạo các thành viên trong group
        for (int i = 0; i < 3; i++) {
            pid_t member = fork();
            
            if (member == 0) {
                // Các con thuộc cùng process group với leader
                printf("[MEMBER%d] PID=%d, PGID=%d\n", 
                       i + 1, getpid(), getpgrp());
                
                sleep(2);
                exit(0);
            }
        }
        
        // Leader đợi các thành viên
        for (int i = 0; i < 3; i++) {
            wait(NULL);
        }
        
        printf("[LEADER] Tất cả thành viên đã kết thúc\n");
        exit(0);
    } else {
        wait(NULL);
    }
}

void example4_send_signal_to_group() {
    printf("\n=== Ví dụ 4: Gửi signal đến toàn bộ group ===\n");
    
    pid_t leader = fork();
    
    if (leader < 0) {
        perror("fork");
        return;
    } else if (leader == 0) {
        // Leader tạo group
        setpgid(0, 0);
        pid_t pgid = getpgrp();
        
        printf("\n[LEADER] PID=%d, PGID=%d\n", getpid(), pgid);
        
        // Tạo worker processes
        for (int i = 0; i < 3; i++) {
            pid_t worker = fork();
            
            if (worker == 0) {
                signal(SIGTERM, SIG_DFL);
                
                printf("[WORKER%d] PID=%d, PGID=%d, đang chờ signal...\n",
                       i + 1, getpid(), getpgrp());
                
                while (1) {
                    sleep(1);
                }
                
                exit(0);
            }
        }
        
        sleep(2);
        
        printf("\n[LEADER] Gửi SIGTERM đến toàn bộ group %d\n", pgid);
        kill(-pgid, SIGTERM);
        
        // Đợi workers
        for (int i = 0; i < 3; i++) {
            int status;
            pid_t pid = wait(&status);
            
            if (WIFSIGNALED(status)) {
                printf("[LEADER] Worker %d bị kill bởi signal %d\n",
                       pid, WTERMSIG(status));
            }
        }
        
        exit(0);
    } else {
        wait(NULL);
    }
}

void example5_session_leader() {
    printf("\n=== Ví dụ 5: Tạo session mới ===\n");
    
    print_ids("Trước khi fork");
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        return;
    } else if (pid == 0) {
        printf("\n[CON] Trước khi tạo session:\n");
        print_ids("Con");
        
        // Tạo session mới
        printf("[CON] Tạo session mới với setsid()...\n");
        
        pid_t sid = setsid();
        if (sid == -1) {
            perror("setsid");
            exit(1);
        }
        
        printf("\n[CON] Sau khi tạo session:\n");
        print_ids("Con (session leader mới)");
        
        printf("[CON] Bây giờ con là:\n");
        printf("  - Session leader (SID = PID)\n");
        printf("  - Process group leader (PGID = PID)\n");
        printf("  - Không có controlling terminal\n");
        
        sleep(2);
        exit(0);
    } else {
        wait(NULL);
        
        printf("\n[CHA] Session không thay đổi:\n");
        print_ids("Cha");
    }
}

void example6_job_control_simulation() {
    printf("\n=== Ví dụ 6: Mô phỏng job control ===\n");
    
    printf("Tạo 2 jobs (process groups) khác nhau\n\n");
    
    // Job 1
    pid_t job1 = fork();
    
    if (job1 < 0) {
        perror("fork");
        return;
    } else if (job1 == 0) {
        setpgid(0, 0);
        
        printf("[JOB1] PID=%d, PGID=%d\n", getpid(), getpgrp());
        printf("[JOB1] Đang chạy...\n");
        
        for (int i = 0; i < 5; i++) {
            printf("[JOB1] Tick %d\n", i + 1);
            sleep(1);
        }
        
        printf("[JOB1] Hoàn thành\n");
        exit(0);
    }
    
    sleep(1);
    
    // Job 2
    pid_t job2 = fork();
    
    if (job2 < 0) {
        perror("fork");
        return;
    } else if (job2 == 0) {
        setpgid(0, 0);
        
        printf("[JOB2] PID=%d, PGID=%d\n", getpid(), getpgrp());
        printf("[JOB2] Đang chạy...\n");
        
        for (int i = 0; i < 3; i++) {
            printf("[JOB2] Tick %d\n", i + 1);
            sleep(1);
        }
        
        printf("[JOB2] Hoàn thành\n");
        exit(0);
    }
    
    printf("\n[SHELL] Đang quản lý 2 jobs\n");
    printf("[SHELL] Job1 PGID=%d\n", job1);
    printf("[SHELL] Job2 PGID=%d\n", job2);
    
    // Đợi jobs
    wait(NULL);
    printf("\n[SHELL] Job đầu tiên hoàn thành\n");
    
    wait(NULL);
    printf("[SHELL] Tất cả jobs hoàn thành\n");
}

void print_menu() {
    printf("\n=== MENU: Process Groups và Sessions ===\n");
    printf("1. Các ID cơ bản\n");
    printf("2. Tạo process group mới\n");
    printf("3. Nhóm tiến trình gia đình\n");
    printf("4. Gửi signal đến toàn bộ group\n");
    printf("5. Tạo session mới\n");
    printf("6. Mô phỏng job control\n");
    printf("0. Thoát\n");
    printf("Chọn: ");
}

int main() {
    printf("=== PROCESS GROUPS AND SESSIONS EXAMPLES ===\n");
    printf("PID: %d\n", getpid());
    
    example1_basic_ids();
    sleep(1);
    
    example2_create_process_group();
    sleep(1);
    
    example3_process_group_family();
    sleep(1);
    
    example4_send_signal_to_group();
    sleep(1);
    
    example5_session_leader();
    sleep(1);
    
    example6_job_control_simulation();
    
    printf("\n=== HOÀN THÀNH ===\n");
    printf("\nTóm tắt process groups & sessions:\n");
    printf("  getpgrp()  - lấy PGID của tiến trình hiện tại\n");
    printf("  getpgid()  - lấy PGID của tiến trình khác\n");
    printf("  setpgid()  - đặt process group mới\n");
    printf("  getsid()   - lấy SID của tiến trình\n");
    printf("  setsid()   - tạo session mới (trở thành leader)\n");
    printf("  kill(-pgid, sig) - gửi signal đến toàn bộ group\n");
    
    return 0;
}
