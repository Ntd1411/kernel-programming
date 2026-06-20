/*
 * daemon.c - Tạo daemon process
 * 
 * Daemon là tiến trình chạy nền, không gắn với terminal
 * 
 * Biên dịch: gcc -o daemon daemon.c
 * Chạy: ./daemon
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include <time.h>

#define DAEMON_NAME "my_daemon"
#define PID_FILE "/tmp/my_daemon.pid"
#define LOG_FILE "/tmp/my_daemon.log"

volatile sig_atomic_t running = 1;

void signal_handler(int signum) {
    if (signum == SIGTERM || signum == SIGINT) {
        running = 0;
    } else if (signum == SIGHUP) {
        // Reload configuration
        syslog(LOG_INFO, "Nhận SIGHUP - reload config");
    }
}

void write_pid_file() {
    FILE *fp = fopen(PID_FILE, "w");
    if (fp != NULL) {
        fprintf(fp, "%d\n", getpid());
        fclose(fp);
    }
}

void remove_pid_file() {
    unlink(PID_FILE);
}

void daemonize() {
    pid_t pid, sid;
    
    // Fork lần 1
    pid = fork();
    
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    
    // Cha thoát
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    
    // Con trở thành session leader
    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }
    
    // Fork lần 2 để đảm bảo không thể mở terminal
    pid = fork();
    
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    
    // Thay đổi working directory
    if (chdir("/") < 0) {
        exit(EXIT_FAILURE);
    }
    
    // Đóng file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    // Mở lại stdin, stdout, stderr đến /dev/null
    open("/dev/null", O_RDONLY); // stdin
    open("/dev/null", O_WRONLY); // stdout
    open("/dev/null", O_WRONLY); // stderr
    
    // Đặt umask
    umask(0);
}

void daemon_work() {
    int counter = 0;
    
    // Mở syslog
    openlog(DAEMON_NAME, LOG_PID | LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "Daemon started, PID=%d", getpid());
    
    // Ghi PID file
    write_pid_file();
    
    // Đăng ký signal handlers
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGHUP, signal_handler);
    
    // Vòng lặp chính
    while (running) {
        counter++;
        
        syslog(LOG_INFO, "Daemon is running... counter=%d", counter);
        
        // Ghi vào log file
        FILE *fp = fopen(LOG_FILE, "a");
        if (fp != NULL) {
            time_t now = time(NULL);
            char *time_str = ctime(&now);
            time_str[strlen(time_str) - 1] = '\0'; // Xóa newline
            
            fprintf(fp, "[%s] Counter: %d\n", time_str, counter);
            fclose(fp);
        }
        
        sleep(5);
    }
    
    syslog(LOG_INFO, "Daemon stopping...");
    
    // Cleanup
    remove_pid_file();
    closelog();
}

void show_usage(const char *prog) {
    printf("Daemon Example\n\n");
    printf("Sử dụng:\n");
    printf("  %s start   - Start daemon\n", prog);
    printf("  %s stop    - Stop daemon\n", prog);
    printf("  %s status  - Check daemon status\n", prog);
    printf("  %s reload  - Reload daemon config\n", prog);
    printf("\n");
    printf("Log file: %s\n", LOG_FILE);
    printf("PID file: %s\n", PID_FILE);
}

int read_pid_file() {
    FILE *fp = fopen(PID_FILE, "r");
    if (fp == NULL) {
        return -1;
    }
    
    int pid;
    if (fscanf(fp, "%d", &pid) != 1) {
        fclose(fp);
        return -1;
    }
    
    fclose(fp);
    return pid;
}

int is_process_running(int pid) {
    // Kiểm tra xem process có tồn tại không
    if (kill(pid, 0) == 0) {
        return 1;
    }
    return 0;
}

void start_daemon() {
    int pid = read_pid_file();
    
    if (pid > 0 && is_process_running(pid)) {
        printf("Daemon đã đang chạy với PID %d\n", pid);
        return;
    }
    
    printf("Starting daemon...\n");
    
    pid_t fork_pid = fork();
    
    if (fork_pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    
    if (fork_pid > 0) {
        // Cha
        printf("Daemon started với PID %d\n", fork_pid);
        printf("Kiểm tra log: tail -f %s\n", LOG_FILE);
        exit(EXIT_SUCCESS);
    }
    
    // Con: daemonize và chạy
    daemonize();
    daemon_work();
    
    exit(EXIT_SUCCESS);
}

void stop_daemon() {
    int pid = read_pid_file();
    
    if (pid < 0) {
        printf("Không tìm thấy PID file. Daemon không chạy?\n");
        return;
    }
    
    printf("Stopping daemon PID %d...\n", pid);
    
    if (kill(pid, SIGTERM) == 0) {
        printf("SIGTERM sent. Đang đợi daemon dừng...\n");
        
        // Đợi daemon dừng
        for (int i = 0; i < 10; i++) {
            sleep(1);
            if (!is_process_running(pid)) {
                printf("Daemon đã dừng\n");
                return;
            }
        }
        
        printf("Daemon không dừng, force kill...\n");
        kill(pid, SIGKILL);
    } else {
        perror("kill");
        printf("Không thể dừng daemon\n");
    }
}

void status_daemon() {
    int pid = read_pid_file();
    
    if (pid < 0) {
        printf("Daemon không chạy (không có PID file)\n");
        return;
    }
    
    if (is_process_running(pid)) {
        printf("Daemon đang chạy với PID %d\n", pid);
    } else {
        printf("Daemon không chạy (stale PID file)\n");
        remove_pid_file();
    }
}

void reload_daemon() {
    int pid = read_pid_file();
    
    if (pid < 0) {
        printf("Daemon không chạy\n");
        return;
    }
    
    printf("Sending SIGHUP to daemon PID %d...\n", pid);
    
    if (kill(pid, SIGHUP) == 0) {
        printf("SIGHUP sent successfully\n");
    } else {
        perror("kill");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        show_usage(argv[0]);
        return 1;
    }
    
    const char *command = argv[1];
    
    if (strcmp(command, "start") == 0) {
        start_daemon();
    } else if (strcmp(command, "stop") == 0) {
        stop_daemon();
    } else if (strcmp(command, "status") == 0) {
        status_daemon();
    } else if (strcmp(command, "reload") == 0) {
        reload_daemon();
    } else {
        printf("Lệnh không hợp lệ: %s\n", command);
        show_usage(argv[0]);
        return 1;
    }
    
    return 0;
}
