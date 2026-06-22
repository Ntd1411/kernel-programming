# Process Management - Quản lý Tiến trình trong Linux

## Tổng quan

Thư mục này chứa các ví dụ về quản lý tiến trình (process management) trong Linux.

**Bao gồm:**
- fork() - Tạo tiến trình con
- exec family - Thay thế process image
- Xử lý signals (SIGCHLD, SIGTERM, SIGINT)
- Quản lý zombie processes
- Process priority & scheduling

## Build và Run

```bash
make all              # Build tất cả
make clean           # Clean object files
./fork_example       # Chạy fork demo
./signal_handler     # Chạy signal handling demo
```

## Các File Chính

| File | Mô Tả |
|------|-------|
| `fork_example.c` | Fork và tạo child process |
| `exec_family.c` | execl, execv, execle, execve |
| `signal_handler.c` | Signal handling |
| `zombie_reaper.c` | SIGCHLD và reaping zombies |
| `process_priority.c` | nice/setpriority |

## Khái Niệm Chính

### Process Lifecycle
- **Creation**: fork()
- **Execution**: exec family
- **Termination**: exit(), wait()
- **Zombie**: Process finished but parent hasn't waited

### Signals
- **SIGCHLD**: Khi child process kết thúc
- **SIGTERM**: Terminate signal
- **SIGINT**: Interrupt signal (Ctrl+C)
- **Signal Masking**: sigprocmask, sigaction

### System Calls
```c
pid_t fork(void);
int execve(const char *pathname, char *const argv[], char *const envp[]);
pid_t wait(int *wstatus);
pid_t waitpid(pid_t pid, int *wstatus, int options);
int kill(pid_t pid, int sig);
```

## Tài Liệu Tham Khảo

```bash
man 2 fork
man 2 execve
man 2 wait
man 2 signal
man 2 kill
```

Xem thêm: [system/README.md](../README.md)
- **PPID (Parent Process ID)**: PID của tiến trình cha
- **Memory space**: Vùng nhớ riêng (text, data, stack, heap)
- **File descriptors**: Các file đang mở
- **Environment variables**: Biến môi trường
- **Scheduling information**: Thông tin lập lịch (priority, nice value)

### 2. Process States (Trạng thái tiến trình)

```
R - Running: Đang chạy hoặc sẵn sàng chạy
S - Sleeping: Đang chờ event (có thể bị interrupt)
D - Disk sleep: Đang chờ I/O (không thể interrupt)
T - Stopped: Bị dừng bởi signal (SIGSTOP, SIGTSTP)
Z - Zombie: Đã kết thúc nhưng chưa được thu hồi
```

### 3. Process Creation Model

Linux sử dụng mô hình **fork-exec**:
1. **fork()**: Tạo bản sao của tiến trình hiện tại
2. **exec()**: Thay thế chương trình hiện tại bằng chương trình mới

---

## Chi tiết từng chương trình

## 1. fork_example.c - Tạo tiến trình với fork()

### Kiến thức sử dụng

#### API chính: `fork()`

```c
#include <unistd.h>
pid_t fork(void);
```

**Cách hoạt động:**
- Tạo tiến trình con là bản sao gần như hoàn toàn của tiến trình cha
- Trả về 2 lần: một lần trong cha (giá trị = PID con), một lần trong con (giá trị = 0)
- Trả về -1 nếu lỗi

**Điểm quan trọng:**
- Con kế thừa: file descriptors, environment variables, signal handlers
- Con có: PID riêng, PPID = PID của cha, bộ nhớ riêng (copy-on-write)
- Con không kế thừa: file locks, pending signals, timers

#### API liên quan: `getpid()`, `getppid()`

```c
pid_t getpid(void);   // Lấy PID của tiến trình hiện tại
pid_t getppid(void);  // Lấy PID của tiến trình cha
```

#### API chờ tiến trình con: `wait()` và `waitpid()`

```c
#include <sys/wait.h>

pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);
```

**wait()**: Chờ bất kỳ con nào kết thúc
**waitpid()**: Chờ con cụ thể, có options linh hoạt hơn

**Status macros:**
- `WIFEXITED(status)`: Con thoát bình thường?
- `WEXITSTATUS(status)`: Lấy exit code
- `WIFSIGNALED(status)`: Con bị kill bởi signal?
- `WTERMSIG(status)`: Signal nào kill con?

### Ví dụ code

```c
pid_t pid = fork();

if (pid < 0) {
    perror("fork");
} else if (pid == 0) {
    // Tiến trình con
    printf("Con: PID=%d, PPID=%d\n", getpid(), getppid());
    exit(0);
} else {
    // Tiến trình cha
    printf("Cha: PID=%d, Con PID=%d\n", getpid(), pid);
    wait(NULL);
}
```

### Copy-on-Write (COW)

- Khi fork(), kernel không copy toàn bộ memory ngay lập tức
- Cha và con chia sẻ cùng physical memory (read-only)
- Chỉ khi một trong hai ghi vào memory thì mới copy (write)
- Tối ưu hiệu suất và tiết kiệm bộ nhớ

---

## 2. exec_family.c - Họ hàm exec

### Kiến thức sử dụng

#### Họ hàm exec

Thay thế chương trình hiện tại bằng chương trình mới. Các biến thể:

```c
#include <unistd.h>

int execl(const char *path, const char *arg, ...);
int execlp(const char *file, const char *arg, ...);
int execle(const char *path, const char *arg, ..., char *const envp[]);

int execv(const char *path, char *const argv[]);
int execvp(const char *file, char *const argv[]);
int execve(const char *path, char *const argv[], char *const envp[]);
```

**Quy tắc đặt tên:**
- **l** (list): Tham số dạng danh sách `arg0, arg1, ..., NULL`
- **v** (vector): Tham số dạng mảng `char *argv[]`
- **p** (path): Tìm file trong PATH
- **e** (environment): Truyền environment variables tùy chỉnh

#### Chi tiết từng hàm

**execl() - List arguments**
```c
execl("/bin/ls", "ls", "-l", "/tmp", NULL);
```

**execlp() - List + PATH search**
```c
execlp("ls", "ls", "-l", "/tmp", NULL);
```

**execle() - List + Environment**
```c
char *envp[] = {"PATH=/bin", "USER=test", NULL};
execle("/bin/ls", "ls", "-l", NULL, envp);
```

**execv() - Vector arguments**
```c
char *argv[] = {"ls", "-l", "/tmp", NULL};
execv("/bin/ls", argv);
```

**execvp() - Vector + PATH search**
```c
char *argv[] = {"ls", "-l", "/tmp", NULL};
execvp("ls", argv);
```

**execve() - Vector + Environment (syscall thực sự)**
```c
char *argv[] = {"ls", "-l", NULL};
char *envp[] = {"PATH=/bin", NULL};
execve("/bin/ls", argv, envp);
```

### Điểm quan trọng

- Nếu exec thành công, code sau đó **không bao giờ chạy**
- Process ID không đổi, chỉ thay đổi chương trình
- File descriptors mở vẫn giữ nguyên (trừ FD_CLOEXEC)
- Signals bị reset về default
- Memory bị thay thế hoàn toàn

### Ví dụ fork-exec pattern

```c
pid_t pid = fork();

if (pid == 0) {
    // Con: thay thế bằng chương trình mới
    execl("/bin/ls", "ls", "-l", NULL);
    perror("execl");  // Chỉ chạy nếu exec lỗi
    exit(1);
} else {
    // Cha: tiếp tục chạy
    wait(NULL);
}
```

---

## 3. signal_handler.c - Xử lý tín hiệu

### Kiến thức sử dụng

#### Signal là gì?

Signal là cơ chế thông báo bất đồng bộ gửi đến tiến trình. Dùng để:
- Thông báo sự kiện (Ctrl+C, timer timeout, child exit)
- Kill/stop tiến trình
- Giao tiếp giữa các tiến trình

#### Các signal phổ biến

```c
SIGINT  (2)   - Interrupt từ keyboard (Ctrl+C)
SIGQUIT (3)   - Quit từ keyboard (Ctrl+\)
SIGKILL (9)   - Kill không thể bắt hoặc ignore
SIGTERM (15)  - Terminate (mềm, có thể bắt)
SIGCHLD (17)  - Con kết thúc hoặc stop
SIGSTOP (19)  - Stop không thể bắt
SIGTSTP (20)  - Stop từ terminal (Ctrl+Z)
SIGCONT (18)  - Continue sau khi stop
SIGUSR1 (10)  - User-defined signal 1
SIGUSR2 (12)  - User-defined signal 2
SIGALRM (14)  - Alarm clock (từ alarm())
SIGSEGV (11)  - Segmentation fault
SIGPIPE (13)  - Broken pipe
```

#### API: `signal()` (cũ, không nên dùng)

```c
#include <signal.h>

typedef void (*sighandler_t)(int);
sighandler_t signal(int signum, sighandler_t handler);
```

**Handlers đặc biệt:**
- `SIG_DFL`: Handler mặc định
- `SIG_IGN`: Ignore signal

#### API: `sigaction()` (khuyên dùng)

```c
#include <signal.h>

int sigaction(int signum, const struct sigaction *act, 
              struct sigaction *oldact);

struct sigaction {
    void (*sa_handler)(int);
    void (*sa_sigaction)(int, siginfo_t *, void *);
    sigset_t sa_mask;
    int sa_flags;
    void (*sa_restorer)(void);
};
```

**Flags quan trọng:**
- `SA_RESTART`: Tự động restart syscall bị interrupt
- `SA_SIGINFO`: Dùng sa_sigaction thay vì sa_handler
- `SA_NODEFER`: Không block signal đang xử lý
- `SA_RESETHAND`: Reset về SIG_DFL sau khi xử lý

#### API gửi signal: `kill()`, `raise()`

```c
#include <signal.h>

int kill(pid_t pid, int sig);  // Gửi signal đến tiến trình khác
int raise(int sig);             // Gửi signal cho chính mình
```

#### API alarm và timer

```c
#include <unistd.h>

unsigned int alarm(unsigned int seconds);  // Đặt SIGALRM sau n giây
```

### Ví dụ signal handler

```c
void signal_handler(int signum) {
    printf("Nhận signal %d\n", signum);
}

int main() {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    
    sigaction(SIGINT, &sa, NULL);
    
    while(1) {
        pause();  // Chờ signal
    }
}
```

### Signal Safety

**Async-signal-safe functions**: Chỉ gọi các hàm an toàn trong signal handler:
- `write()`, `_exit()`, `signal()`, `sigaction()`
- KHÔNG an toàn: `printf()`, `malloc()`, `free()`

---

## 4. zombie_reaper.c - Xử lý tiến trình zombie

### Kiến thức sử dụng

#### Zombie Process là gì?

Zombie là tiến trình đã kết thúc nhưng:
- Process descriptor vẫn còn trong kernel
- Chờ cha đọc exit status bằng `wait()`
- Không tiêu tốn CPU/memory nhưng chiếm PID

```
Vòng đời process:
Running -> Exit -> Zombie -> (wait()) -> Hoàn toàn xóa
```

#### Nguyên nhân zombie

```c
// Code tạo zombie
pid_t pid = fork();
if (pid == 0) {
    exit(0);  // Con kết thúc
}
// Cha KHÔNG gọi wait() -> Con trở thành zombie
sleep(100);
```

#### Cách phòng tránh zombie

**Phương pháp 1: Gọi wait() trong cha**
```c
pid_t pid = fork();
if (pid > 0) {
    wait(NULL);  // Thu hồi con
}
```

**Phương pháp 2: SIGCHLD handler**
```c
void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

signal(SIGCHLD, sigchld_handler);
```

**Phương pháp 3: Double fork**
```c
if (fork() == 0) {
    if (fork() == 0) {
        // Cháu: làm việc thực sự
        sleep(5);
        exit(0);
    }
    exit(0);  // Con thoát ngay
}
wait(NULL);  // Thu hồi con, cháu được init nhận nuôi
```

**Phương pháp 4: Ignore SIGCHLD**
```c
signal(SIGCHLD, SIG_IGN);  // Kernel tự động thu hồi
```

#### API quan trọng

```c
// waitpid options
WNOHANG   - Trả về ngay nếu không có con nào kết thúc
WUNTRACED - Báo cáo con bị stopped
WCONTINUED - Báo cáo con được continued
```

---

## 5. process_priority.c - Quản lý độ ưu tiên

### Kiến thức sử dụng

#### Nice Value

Nice value xác định độ ưu tiên tiến trình:
- Giá trị: -20 (cao nhất) đến +19 (thấp nhất)
- Mặc định: 0
- Chỉ root mới giảm nice value (tăng priority)

#### API: `nice()`, `getpriority()`, `setpriority()`

```c
#include <unistd.h>

int nice(int inc);  // Tăng nice value thêm inc

#include <sys/resource.h>

int getpriority(int which, id_t who);
int setpriority(int which, id_t who, int prio);
```

**which parameter:**
- `PRIO_PROCESS`: Áp dụng cho process
- `PRIO_PGRP`: Áp dụng cho process group
- `PRIO_USER`: Áp dụng cho user

#### Scheduling Policies

Linux hỗ trợ nhiều chính sách lập lịch:

```c
#include <sched.h>

int sched_setscheduler(pid_t pid, int policy, 
                       const struct sched_param *param);
int sched_getscheduler(pid_t pid);

struct sched_param {
    int sched_priority;
};
```

**Policies:**
- `SCHED_OTHER` (SCHED_NORMAL): Mặc định, dùng nice value
- `SCHED_FIFO`: Real-time, FIFO (cần root)
- `SCHED_RR`: Real-time, Round-robin (cần root)
- `SCHED_BATCH`: Batch processing, thông lượng cao
- `SCHED_IDLE`: Priority thấp nhất

#### Real-time Priority

Real-time processes có priority 1-99:
- Cao hơn tất cả SCHED_OTHER processes
- SCHED_FIFO: Chạy cho đến khi block hoặc yield
- SCHED_RR: Giống FIFO nhưng có time slice

```c
struct sched_param param;
param.sched_priority = 50;
sched_setscheduler(0, SCHED_FIFO, &param);
```

#### API lấy thông tin

```c
#include <sched.h>

int sched_get_priority_max(int policy);  // Priority cao nhất
int sched_get_priority_min(int policy);  // Priority thấp nhất
```

### Ví dụ thay đổi priority

```c
// Tăng nice value (giảm priority)
nice(10);

// Đặt priority cụ thể
setpriority(PRIO_PROCESS, 0, 5);

// Lấy priority hiện tại
int prio = getpriority(PRIO_PROCESS, 0);
printf("Priority: %d\n", prio);
```

---

## Khái niệm nâng cao

### Process Group và Session

- **Process Group**: Nhóm các processes liên quan (cùng job)
- **Session**: Nhóm các process groups (thường từ một login)
- **Controlling Terminal**: Terminal điều khiển session

```c
#include <unistd.h>

pid_t getpgrp(void);           // Lấy process group ID
int setpgid(pid_t pid, pid_t pgid);  // Đặt process group
pid_t getsid(pid_t pid);       // Lấy session ID
pid_t setsid(void);            // Tạo session mới
```

### Orphan Process

- Tiến trình có cha bị kill
- Được init (PID 1) nhận nuôi
- Không phải zombie

### Daemon Process

Tiến trình chạy nền, không có controlling terminal:
1. Fork và cha exit (tách khỏi terminal)
2. `setsid()` (tạo session mới)
3. `chdir("/")` (đổi working directory)
4. Close file descriptors
5. Redirect stdin/stdout/stderr đến /dev/null

### Resource Limits (rlimit)

Giới hạn tài nguyên cho process:

```c
#include <sys/resource.h>

int getrlimit(int resource, struct rlimit *rlim);
int setrlimit(int resource, const struct rlimit *rlim);

struct rlimit {
    rlim_t rlim_cur;  // Soft limit
    rlim_t rlim_max;  // Hard limit
};
```

**Resources:**
- `RLIMIT_CPU`: Thời gian CPU (giây)
- `RLIMIT_FSIZE`: Kích thước file tối đa
- `RLIMIT_DATA`: Kích thước data segment
- `RLIMIT_STACK`: Kích thước stack
- `RLIMIT_NOFILE`: Số file descriptors
- `RLIMIT_NPROC`: Số processes

---

## Tổng kết các API quan trọng

### Process Creation & Execution
- `fork()` - Tạo tiến trình con
- `execl()`, `execv()`, `execvp()`, ... - Thay thế chương trình
- `exit()`, `_exit()` - Kết thúc tiến trình
- `wait()`, `waitpid()` - Chờ tiến trình con

### Process Information
- `getpid()`, `getppid()` - Lấy PID
- `getuid()`, `geteuid()` - Lấy user ID
- `getgid()`, `getegid()` - Lấy group ID

### Signals
- `signal()` - Đăng ký signal handler (cũ)
- `sigaction()` - Đăng ký signal handler (mới)
- `kill()` - Gửi signal
- `raise()` - Gửi signal cho chính mình
- `alarm()` - Đặt alarm timer
- `pause()` - Chờ signal

### Priority & Scheduling
- `nice()` - Thay đổi nice value
- `getpriority()`, `setpriority()` - Get/set priority
- `sched_setscheduler()` - Đặt scheduling policy
- `sched_getscheduler()` - Lấy scheduling policy

### Process Groups & Sessions
- `getpgrp()`, `setpgid()` - Process group
- `getsid()`, `setsid()` - Session

### Resource Limits
- `getrlimit()`, `setrlimit()` - Resource limits

---

## Best Practices

### 1. Luôn kiểm tra return value

```c
pid_t pid = fork();
if (pid < 0) {
    perror("fork");
    exit(1);
}
```

### 2. Xử lý SIGCHLD để tránh zombie

```c
void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

signal(SIGCHLD, sigchld_handler);
```

### 3. Dùng sigaction() thay vì signal()

```c
struct sigaction sa;
sa.sa_handler = handler;
sa.sa_flags = SA_RESTART;
sigemptyset(&sa.sa_mask);
sigaction(SIGINT, &sa, NULL);
```

### 4. Cẩn thận với signal handlers

- Chỉ dùng async-signal-safe functions
- Không gọi malloc(), printf() trong handler
- Dùng volatile sig_atomic_t cho shared variables

### 5. Cleanup resources trong tiến trình con

```c
if (fork() == 0) {
    // Đóng file descriptors không cần
    close(fd);
    // Làm việc
    exit(0);  // Luôn exit rõ ràng
}
```

---

## Debugging Tips

### 1. Xem process tree

```bash
pstree -p          # Hiển thị cây tiến trình
ps -ef             # Liệt kê tất cả processes
ps aux             # Chi tiết hơn
```

### 2. Tìm zombie processes

```bash
ps aux | grep Z    # Tìm processes có state Z
```

### 3. Gửi signal đến process

```bash
kill -SIGTERM <pid>   # Gửi SIGTERM
kill -9 <pid>         # Gửi SIGKILL (force kill)
killall <name>        # Kill theo tên
```

### 4. Thay đổi priority

```bash
nice -n 10 ./program      # Chạy với nice +10
renice -n 5 -p <pid>      # Thay đổi nice của process đang chạy
```

### 5. Xem signal handlers

```bash
cat /proc/<pid>/status | grep Sig
```

---

## Tài liệu tham khảo

- `man 2 fork` - fork manual
- `man 3 exec` - exec family manual
- `man 7 signal` - signal overview
- `man 2 wait` - wait/waitpid manual
- `man 2 sched_setscheduler` - scheduling manual
- Advanced Programming in the UNIX Environment (APUE)
- The Linux Programming Interface (TLPI)

---

## Lưu ý

- Các ví dụ cần chạy trên Linux (không chạy trên Windows)
- Một số tính năng yêu cầu quyền root (real-time scheduling)
- Test kỹ trước khi dùng trong production
- Cẩn thận với race conditions khi làm việc với signals

