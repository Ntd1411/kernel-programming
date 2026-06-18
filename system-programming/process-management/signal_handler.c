/*
 * signal_handler.c - Xử lý signals trong Linux
 * 
 * Signals là cơ chế IPC để gửi thông báo cho tiến trình
 * 
 * Biên dịch: gcc -o signal_handler signal_handler.c
 * Chạy: ./signal_handler
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

volatile sig_atomic_t got_signal = 0;
volatile sig_atomic_t signal_count = 0;

// Handler cho SIGINT (Ctrl+C)
void sigint_handler(int signum) {
    got_signal = signum;
    signal_count++;
    
    // Chỉ sử dụng async-signal-safe functions
    const char msg[] = "\n[SIGNAL] Nhận SIGINT (Ctrl+C)! Nhấn 3 lần để thoát.\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    
    if (signal_count >= 3) {
        const char exit_msg[] = "[SIGNAL] Thoát chương trình...\n";
        write(STDOUT_FILENO, exit_msg, sizeof(exit_msg) - 1);
        exit(0);
    }
}

// Handler cho SIGTERM
void sigterm_handler(int signum) {
    const char msg[] = "\n[SIGNAL] Nhận SIGTERM! Dọn dẹp và thoát...\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    exit(0);
}

// Handler cho SIGUSR1
void sigusr1_handler(int signum) {
    const char msg[] = "\n[SIGNAL] Nhận SIGUSR1!\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

// Handler cho SIGUSR2
void sigusr2_handler(int signum) {
    const char msg[] = "\n[SIGNAL] Nhận SIGUSR2!\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

// Handler cho SIGALRM
void sigalrm_handler(int signum) {
    const char msg[] = "\n[SIGNAL] Alarm timeout!\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

void example1_basic_signal() {
    printf("\n=== Ví dụ 1: Signal handler cơ bản ===\n");
    printf("PID: %d\n", getpid());
    printf("Nhấn Ctrl+C để gửi SIGINT (tối đa 3 lần)\n\n");
    
    // Đăng ký signal handler
    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigterm_handler);
    
    // Reset counter
    signal_count = 0;
    
    // Vòng lặp với timeout
    int timeout = 10;
    while (timeout > 0 && signal_count < 3) {
        printf("Đang chạy... (signal_count=%d, còn %d giây)\n", signal_count, timeout);
        sleep(1);
        timeout--;
    }
    
    if (signal_count == 0) {
        printf("Hết thời gian, chuyển ví dụ tiếp theo\n");
    }
}

void example2_sigaction() {
    printf("\n=== Ví dụ 2: sigaction() (khuyến nghị hơn signal()) ===\n");
    printf("PID: %d\n", getpid());
    
    struct sigaction sa;
    
    // Cấu hình sigaction
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigusr1_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction SIGUSR1");
        exit(EXIT_FAILURE);
    }
    
    sa.sa_handler = sigusr2_handler;
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("sigaction SIGUSR2");
        exit(EXIT_FAILURE);
    }
    
    printf("Đã đăng ký handler cho SIGUSR1 và SIGUSR2\n");
    printf("Có thể test bằng: kill -SIGUSR1 %d hoặc kill -SIGUSR2 %d\n", getpid(), getpid());
    printf("Đợi 5 giây...\n\n");
    
    sleep(5);
    printf("Hoàn thành ví dụ 2\n");
}

void example3_alarm() {
    printf("\n=== Ví dụ 3: alarm() và SIGALRM ===\n");
    
    signal(SIGALRM, sigalrm_handler);
    
    printf("Đặt alarm 3 giây\n");
    alarm(3);
    
    printf("Đang đợi...\n");
    pause(); // Đợi signal
    
    printf("Alarm đã kích hoạt!\n");
}

void example4_signal_blocking() {
    printf("\n=== Ví dụ 4: Block và unblock signals ===\n");
    
    sigset_t newmask, oldmask;
    
    // Khởi tạo signal set
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGINT);
    
    // Block SIGINT
    printf("Block SIGINT trong 5 giây\n");
    printf("Thử nhấn Ctrl+C - signal sẽ bị block\n\n");
    
    if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0) {
        perror("sigprocmask block");
        exit(EXIT_FAILURE);
    }
    
    for (int i = 5; i > 0; i--) {
        printf("Còn %d giây... (SIGINT đang bị block)\n", i);
        sleep(1);
    }
    
    // Unblock SIGINT
    printf("\nUnblock SIGINT\n");
    if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0) {
        perror("sigprocmask unblock");
        exit(EXIT_FAILURE);
    }
    
    printf("Đã unblock SIGINT\n");
}

void example5_send_signal() {
    printf("\n=== Ví dụ 5: Gửi signal giữa các tiến trình ===\n");
    
    signal(SIGUSR1, sigusr1_handler);
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Con
        printf("[CON] PID=%d, PPID=%d\n", getpid(), getppid());
        printf("[CON] Đợi signal từ cha...\n");
        
        pause(); // Đợi signal
        
        printf("[CON] Đã nhận signal! Gửi signal lại cho cha...\n");
        kill(getppid(), SIGUSR1);
        
        exit(0);
    } else {
        // Cha
        printf("[CHA] PID=%d, Con PID=%d\n", getpid(), pid);
        printf("[CHA] Đợi 2 giây rồi gửi SIGUSR1 cho con...\n");
        
        sleep(2);
        kill(pid, SIGUSR1);
        
        printf("[CHA] Đã gửi signal, đợi signal từ con...\n");
        pause();
        
        printf("[CHA] Đã nhận signal từ con!\n");
        wait(NULL);
    }
}

int main() {
    printf("=== SIGNAL HANDLER EXAMPLES ===\n");
    printf("PID: %d\n", getpid());
    
    example1_basic_signal();
    sleep(1);
    
    example2_sigaction();
    sleep(1);
    
    example3_alarm();
    sleep(1);
    
    example4_signal_blocking();
    sleep(1);
    
    example5_send_signal();
    
    printf("\n=== HOÀN THÀNH ===\n");
    printf("\nTóm tắt signal handling:\n");
    printf("  signal()      - đăng ký signal handler đơn giản\n");
    printf("  sigaction()   - đăng ký handler nâng cao (khuyến nghị)\n");
    printf("  kill()        - gửi signal cho tiến trình khác\n");
    printf("  alarm()       - đặt timer gửi SIGALRM\n");
    printf("  pause()       - đợi signal\n");
    printf("  sigprocmask() - block/unblock signals\n");
    printf("  Lưu ý: Chỉ dùng async-signal-safe functions trong handler\n");
    
    return 0;
}
