# Phần 2: Lập Trình Hệ Thống

Lập trình C/C++ tương tác trực tiếp với Linux kernel thông qua system calls.

## Mục Tiêu

- Hiểu và sử dụng system calls
- Quản lý tiến trình (process)
- Xử lý file với low-level I/O
- Lập trình socket và network
- Xử lý signals và IPC

## Cấu Trúc

### 1. Process Management (process-management/)

Quản lý tiến trình:

Cơ bản (Bắt đầu từ đây):
fork_example - Fork cơ bản
exec_family - Exec functions
signal_handler - Signal handling
zombie_reaper - Zombie processes
Trung bình:
process_groups - Process groups và sessions
process_info - Đọc thông tin process
Nâng cao:
process_priority - Nice value và scheduling (cần sudo cho real-time)
cpu_affinity - CPU affinity (cần multi-core)
ipc_basics - IPC đầy đủ (phức tạp nhất)
daemon - Daemon process

### 2. File I/O (file-io/)

Thao tác file:

- `file_operations.c` - Open, read, write, close
- `file_locking.c` - File locking với fcntl
- `memory_map.c` - Memory-mapped I/O (mmap)
- `directory_walk.c` - Duyệt thư mục
- `inotify_example.c` - Giám sát thay đổi file

### 3. Socket Programming (socket-programming/)

Lập trình socket:

- `tcp_server.c` - TCP server
- `tcp_client.c` - TCP client
- `udp_server.c` - UDP server
- `udp_client.c` - UDP client
- `echo_server.c` - Echo server đa client

### 4. Network Programming (network-programming/)

Lập trình mạng nâng cao:

- `select_server.c` - Server dùng select()
- `poll_server.c` - Server dùng poll()
- `epoll_server.c` - Server dùng epoll()
- `raw_socket.c` - Raw socket programming
- `packet_sniffer.c` - Bắt gói tin mạng

## Biên Dịch

### Single file

```bash
gcc -o program program.c
gcc -o program program.c -lpthread  # Nếu dùng threads
```

### Với Makefile

```bash
cd process-management/
make              # Build tất cả
make clean        # Dọn dẹp
make test         # Xem hướng dẫn test
make help         # Xem trợ giúp
./fork_example    # Chạy chương trình
```

## Hướng Dẫn Sử Dụng

### 1. Process Management

#### Fork Example

```bash
cd process-management/
make fork_example
./fork_example
```

Chương trình demo:

- Fork cơ bản với tiến trình cha và con
- Tạo nhiều tiến trình con
- Fork tree (cây tiến trình)
- Chia sẻ và copy-on-write

#### Exec Family

```bash
make exec_family
./exec_family
```

Demo các hàm exec:

- `execl()` - Exec với list arguments
- `execv()` - Exec với vector
- `execle()` - Exec với environment
- `execvp()` - Exec với PATH search

#### Signal Handler

```bash
make signal_handler
./signal_handler 1    # Basic signal (Ctrl+C)
./signal_handler 2    # sigaction()
./signal_handler 3    # alarm và timeout
./signal_handler 4    # signal blocking/masking
./signal_handler 5    # send signal giữa processes
```

Signals được xử lý:

- `SIGINT` (Ctrl+C) - Interrupt
- `SIGTERM` - Terminate
- `SIGUSR1`, `SIGUSR2` - User-defined
- `SIGALRM` - Alarm timeout

#### Zombie Reaper

```bash
make zombie_reaper
./zombie_reaper 1     # Tạo zombie process
./zombie_reaper 2     # Auto-reap với SIGCHLD
./zombie_reaper 3     # waitpid với WNOHANG
./zombie_reaper 4     # Wait specific child
./zombie_reaper 5     # Double fork technique
```

#### Daemon Process

```bash
make daemon
./daemon start        # Start daemon
./daemon status       # Check status
./daemon stop         # Stop daemon

# Xem log
tail -f /tmp/my_daemon.log

# Reload config
kill -HUP $(cat /tmp/my_daemon.pid)
```

### 2. File I/O

```bash
cd ../file-io/
make
```

#### File Operations

```bash
# Ghi file
./file_operations write test.txt "Hello World"

# Đọc file
./file_operations read test.txt

# Thêm vào cuối file
./file_operations append test.txt "\nNew line"

# Copy file
./file_operations copy test.txt test2.txt

# Xem thông tin file
./file_operations stat test.txt
```

#### File Locking

```bash
# Terminal 1: Khóa file và giữ
./file_locking exclusive test.txt
# Nhập text, file sẽ bị khóa

# Terminal 2: Thử ghi vào file đã khóa
./file_locking exclusive test.txt
# Sẽ bị block hoặc báo lỗi

# Shared lock (nhiều reader)
./file_locking shared test.txt
```

#### Memory Map

```bash
# Tạo và map file
./memory_map create testmap.dat 1024

# Đọc mapped file
./memory_map read testmap.dat

# Ghi vào mapped file
./memory_map write testmap.dat "Memory mapped data"

# Copy file bằng mmap
./memory_map copy source.txt dest.txt
```

#### Directory Walk

```bash
# Liệt kê thư mục
./directory_walk list /path/to/dir

# Đếm files và dirs
./directory_walk count /path/to/dir

# Tìm file theo pattern
./directory_walk find /path/to/dir "*.c"

# Recursive walk
./directory_walk recursive /path/to/dir
```

#### Inotify Example

```bash
# Giám sát thư mục hiện tại
./inotify_example .

# Giám sát nhiều paths
./inotify_example /tmp /var/log

# Trong terminal khác, thử:
touch test.txt
echo "data" > test.txt
rm test.txt
```

### 3. Socket Programming

```bash
cd ../socket-programming/
make
```

#### TCP Server/Client

```bash
# Terminal 1: Start server
./tcp_server 8080

# Terminal 2: Connect client
./tcp_client localhost 8080
# Nhập text để gửi đến server

# Hoặc dùng telnet
telnet localhost 8080
```

#### UDP Server/Client

```bash
# Terminal 1: Start UDP server
./udp_server 8081

# Terminal 2: Send UDP messages
./udp_client localhost 8081
# Nhập text để gửi

# Test với netcat
echo "Hello UDP" | nc -u localhost 8081
```

#### Echo Server (Multi-threaded)

```bash
# Start echo server
./echo_server 8082

# Connect nhiều clients
# Terminal 2
./tcp_client localhost 8082

# Terminal 3
./tcp_client localhost 8082

# Terminal 4
telnet localhost 8082
```

### 4. Network Programming

```bash
cd ../network-programming/
make
```

#### Select Server

```bash
# Start server
./select_server 9001

# Connect nhiều clients
telnet localhost 9001
nc localhost 9001
./tcp_client localhost 9001
```

Đặc điểm:

- Xử lý đồng thời nhiều connections
- Timeout support
- Giới hạn: FD_SETSIZE (thường 1024)

#### Poll Server

```bash
# Start server
./poll_server 9002

# Test tương tự select_server
telnet localhost 9002
```

Đặc điểm:

- Không giới hạn số connections như select
- API dễ sử dụng hơn select

#### Epoll Server

```bash
# Start server (Linux only)
./epoll_server 9003

# Benchmark với nhiều connections
for i in {1..100}; do
    (echo "Client $i" | nc localhost 9003 &)
done
```

Đặc điểm:

- Linux-specific
- Rất hiệu quả với hàng ngàn connections
- Edge-triggered và level-triggered mode

#### Raw Socket

```bash
# Cần quyền root
sudo ./raw_socket 8.8.8.8

# Gửi custom ICMP packet
sudo ./raw_socket google.com
```

#### Packet Sniffer

```bash
# Bắt packets trên interface (cần root)
sudo ./packet_sniffer eth0

# Trong terminal khác, tạo traffic
ping google.com
curl http://example.com

# Stop với Ctrl+C để xem statistics
```

## System Calls Quan Trọng

### Process Management

- `fork()` - Tạo tiến trình con
- `exec()` family - Thực thi chương trình
- `wait()`, `waitpid()` - Đợi tiến trình con
- `kill()`, `signal()` - Gửi/xử lý signals
- `getpid()`, `getppid()` - Lấy PID

### File I/O

- `open()`, `creat()` - Mở/tạo file
- `read()`, `write()` - Đọc/ghi
- `close()` - Đóng file
- `lseek()` - Di chuyển file pointer
- `fcntl()` - File control operations
- `ioctl()` - Device I/O control

### Socket Programming

- `socket()` - Tạo socket
- `bind()` - Gắn địa chỉ
- `listen()` - Lắng nghe kết nối
- `accept()` - Chấp nhận kết nối
- `connect()` - Kết nối đến server
- `send()`, `recv()` - Gửi/nhận dữ liệu

### I/O Multiplexing

- `select()` - Giám sát nhiều file descriptors
- `poll()` - Tương tự select nhưng hiệu quả hơn
- `epoll()` - Linux-specific, rất hiệu quả

## Ví Dụ Chương Trình

### Simple TCP Server

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);
    
    client_fd = accept(server_fd, (struct sockaddr*)&address, &addrlen);
    
    char buffer[1024] = {0};
    read(client_fd, buffer, 1024);
    printf("Received: %s\n", buffer);
    
    close(client_fd);
    close(server_fd);
    return 0;
}
```

## Test và Benchmark

### Test TCP Performance

```bash
# Terminal 1: Start server
./tcp_server 8080

# Terminal 2: Benchmark với ab (Apache Bench)
# Cài đặt: apt install apache2-utils
echo "GET / HTTP/1.0\r\n\r\n" | nc localhost 8080

# Stress test với multiple connections
for i in {1..50}; do
    (echo "Test $i" | nc localhost 8080 &)
done
```

### Test I/O Multiplexing Performance

```bash
# So sánh select vs poll vs epoll
time ./select_server 9001 &
time ./poll_server 9002 &
time ./epoll_server 9003 &

# Test với 1000 concurrent connections
for port in 9001 9002 9003; do
    echo "Testing port $port"
    for i in {1..1000}; do
        (echo "test" | nc localhost $port &)
    done
    sleep 5
    pkill nc
done
```

### Memory Leak Check

```bash
# Cài valgrind: apt install valgrind
valgrind --leak-check=full --show-leak-kinds=all ./tcp_server 8080

# Trong terminal khác
./tcp_client localhost 8080
# Ctrl+C server và xem kết quả
```

### System Call Tracing

```bash
# Trace system calls
strace -c ./fork_example          # Count summary
strace -tt ./tcp_server 8080      # With timestamps
strace -e trace=network ./tcp_client localhost 8080  # Only network calls
strace -e trace=file ./file_operations read test.txt # Only file calls
```

### Process Monitoring

```bash
# Giám sát processes
watch -n 1 'ps aux | grep my_daemon'

# Xem open file descriptors
lsof -p $(pgrep tcp_server)

# Network connections
netstat -tulpn | grep 8080
ss -tulpn | grep 8080

# Resource usage
top -p $(pgrep echo_server)
```

## Bài Tập Thực Hành

### Cơ Bản

1. **Fork Pipeline**: Tạo pipeline processes A -> B -> C, mỗi process xử lý và forward data
2. **Signal Logger**: Xây dựng chương trình log tất cả signals nhận được
3. **File Monitor**: Giám sát thư mục và log mọi thay đổi
4. **Simple Shell**: Viết shell đơn giản có thể chạy commands và pipes

### Trung Bình

5. **Chat Server**: Tạo chat server/client với nhiều rooms
   - Multi-threaded hoặc epoll
   - Broadcast messages
   - Private messages
   - User list

2. **HTTP Server**: Xây dựng HTTP server cơ bản
   - Xử lý GET/POST requests
   - Serve static files
   - Multi-threaded connection handling
   - Keep-alive support

3. **File Transfer**: Protocol truyền file qua socket
   - Checksum validation
   - Resume capability
   - Progress tracking
   - Compression

4. **Process Pool**: Worker pool xử lý tasks
   - Master-worker architecture
   - Load balancing
   - Graceful shutdown
   - Health checking

### Nâng Cao

9. **Mini Redis**: Key-value store đơn giản
   - In-memory storage
   - Socket server với protocol
   - Commands: GET, SET, DEL, EXISTS
   - Persistence to disk

2. **Log Aggregator**: Thu thập logs từ nhiều sources
    - UDP log receiver
    - File rotation
    - Search và filter
    - Real-time monitoring

3. **Reverse Proxy**: Simple reverse proxy
    - Forward requests to backend
    - Load balancing
    - Health checks
    - Connection pooling

4. **Container Runtime**: Mini container runtime
    - Process isolation với namespaces
    - Resource limits với cgroups
    - Filesystem isolation
    - Network namespace

## Debug và Troubleshooting

### Compilation Issues

```bash
# Compile với debug symbols
gcc -g -o program program.c

# Với warnings đầy đủ
gcc -Wall -Wextra -Werror -o program program.c

# Check dependencies
ldd ./program

# Static analysis
gcc -fanalyzer -o program program.c
```

### Debugging với GDB

```bash
# Start debugger
gdb ./program

# GDB commands
(gdb) break main              # Set breakpoint
(gdb) run                     # Run program
(gdb) next                    # Next line
(gdb) step                    # Step into function
(gdb) print variable          # Print variable
(gdb) backtrace              # Stack trace
(gdb) info threads           # Thread info
(gdb) attach <pid>           # Attach to running process

# Debug với arguments
gdb --args ./tcp_server 8080

# Debug core dump
ulimit -c unlimited
gdb ./program core
```

### Common Errors và Solutions

#### 1. Address Already in Use

```bash
# Error: bind: Address already in use
# Solution: Enable SO_REUSEADDR
int opt = 1;
setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

# Hoặc kill process đang dùng port
lsof -ti:8080 | xargs kill -9
```

#### 2. Too Many Open Files

```bash
# Error: accept: Too many open files
# Check limit
ulimit -n

# Increase limit (temporary)
ulimit -n 4096

# Permanent: edit /etc/security/limits.conf
* soft nofile 4096
* hard nofile 10240
```

#### 3. Broken Pipe

```bash
# Error: send: Broken pipe (SIGPIPE)
# Solution: Ignore SIGPIPE signal
signal(SIGPIPE, SIG_IGN);

# Hoặc check send() return value
ssize_t sent = send(fd, buf, len, MSG_NOSIGNAL);
```

#### 4. Zombie Processes

```bash
# Check zombies
ps aux | grep Z

# Solution: Wait for children
signal(SIGCHLD, SIG_IGN);  // Auto-reap
# Hoặc
while (waitpid(-1, NULL, WNOHANG) > 0);
```

#### 5. Segmentation Fault

```bash
# Run với valgrind
valgrind --leak-check=full ./program

# Enable core dumps
ulimit -c unlimited
# Run program, khi crash sẽ tạo core file
gdb ./program core

# Check với AddressSanitizer
gcc -fsanitize=address -g -o program program.c
./program
```

#### 6. Race Conditions

```bash
# Compile với thread sanitizer
gcc -fsanitize=thread -g -o program program.c -lpthread
./program

# Use proper synchronization
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_lock(&lock);
// critical section
pthread_mutex_unlock(&lock);
```

### Performance Profiling

```bash
# CPU profiling với perf
perf record ./program
perf report

# Memory profiling
valgrind --tool=massif ./program
ms_print massif.out.*

# System call profiling
strace -c -S time ./program

# Network profiling
tcpdump -i lo port 8080 -w capture.pcap
wireshark capture.pcap
```

### Network Debugging

```bash
# Check listening ports
netstat -tulpn
ss -tulpn

# Test connectivity
telnet localhost 8080
nc -zv localhost 8080

# Send test data
echo -e "GET / HTTP/1.0\r\n\r\n" | nc localhost 8080

# Monitor traffic
tcpdump -i any port 8080 -A

# Check socket options
ss -o state established '( dport = :8080 or sport = :8080 )'
```

## Tài Liệu Tham Khảo

### Sách

- **The Linux Programming Interface** - Michael Kerrisk (Bible của Linux programming)
- **Advanced Programming in the UNIX Environment** - W. Richard Stevens
- **Unix Network Programming** - W. Richard Stevens
- **Linux System Programming** - Robert Love
- **The Art of UNIX Programming** - Eric S. Raymond

### Online Resources

- **Man Pages**: `man 2 <syscall>` - Documentation chi tiết
- **Beej's Guide to Network Programming** - Tutorial socket programming tuyệt vời
- **Linux Kernel Documentation** - <https://www.kernel.org/doc/>
- **The Linux Kernel Archives** - <https://kernel.org/>

### Man Pages Quan Trọng

```bash
# System calls
man 2 fork
man 2 exec
man 2 socket
man 2 select
man 2 epoll

# Library functions
man 3 pthread_create
man 3 printf

# File formats
man 5 proc

# Miscellaneous
man 7 signal
man 7 socket
man 7 ip
```

### Useful Commands

```bash
# Tìm system call
apropos socket
man -k network

# Xem header file location
echo | gcc -E -Wp,-v -

# Check POSIX compliance
getconf -a | grep POSIX
```

## Tips và Best Practices

### 1. Error Handling

```c
// Luôn check return values
if (socket_fd < 0) {
    perror("socket");  // Print error message
    fprintf(stderr, "Error code: %d\n", errno);
    exit(EXIT_FAILURE);
}

// Hoặc dùng macro
#define CHECK(call) \
    if ((call) < 0) { \
        perror(#call); \
        exit(EXIT_FAILURE); \
    }

CHECK(socket(AF_INET, SOCK_STREAM, 0));
```

### 2. Resource Management

```c
// Luôn đóng file descriptors
int fd = open("file.txt", O_RDONLY);
if (fd < 0) return -1;

// ... use fd ...

close(fd);  // Đừng quên!

// Better: Use cleanup attribute (GCC)
void cleanup_fd(int *fd) {
    if (*fd >= 0) close(*fd);
}

int fd __attribute__((cleanup(cleanup_fd))) = open("file.txt", O_RDONLY);
```

### 3. Signal Safety

```c
// Chỉ dùng async-signal-safe functions trong signal handler
volatile sig_atomic_t flag = 0;

void handler(int sig) {
    flag = 1;  // OK
    // printf("Signal!\n");  // NOT OK - not async-safe
    write(STDOUT_FILENO, "Signal!\n", 8);  // OK
}
```

### 4. Thread Safety

```c
// Sử dụng mutex cho shared data
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int shared_counter = 0;

void* thread_func(void *arg) {
    pthread_mutex_lock(&lock);
    shared_counter++;
    pthread_mutex_unlock(&lock);
    return NULL;
}
```

### 5. Buffer Overflow Prevention

```c
// Xấu
char buf[10];
strcpy(buf, user_input);  // Nguy hiểm!

// Tốt
char buf[10];
strncpy(buf, user_input, sizeof(buf) - 1);
buf[sizeof(buf) - 1] = '\0';

// Tốt hơn
snprintf(buf, sizeof(buf), "%s", user_input);
```

### 6. Non-blocking I/O

```c
// Set non-blocking mode
int flags = fcntl(fd, F_GETFL, 0);
fcntl(fd, F_SETFL, flags | O_NONBLOCK);

// Handle EAGAIN/EWOULDBLOCK
ssize_t n = read(fd, buf, sizeof(buf));
if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    // Resource temporarily unavailable, try again later
}
```

### 7. Graceful Shutdown

```c
volatile sig_atomic_t running = 1;

void sigterm_handler(int sig) {
    running = 0;
}

int main() {
    signal(SIGTERM, sigterm_handler);
    signal(SIGINT, sigterm_handler);
    
    while (running) {
        // Main loop
    }
    
    // Cleanup
    return 0;
}
```

## Performance Tips

### 1. Reduce System Calls

```c
// Xấu: Nhiều write() calls
for (int i = 0; i < 1000; i++) {
    write(fd, &data[i], 1);
}

// Tốt: Một write() call
write(fd, data, 1000);
```

### 2. Use Buffering

```c
// stdio buffering
FILE *fp = fopen("file.txt", "w");
setvbuf(fp, NULL, _IOFBF, 8192);  // 8KB buffer
```

### 3. Zero-Copy với sendfile()

```c
// Traditional copy
char buf[8192];
while ((n = read(in_fd, buf, sizeof(buf))) > 0) {
    write(out_fd, buf, n);
}

// Zero-copy
off_t offset = 0;
sendfile(out_fd, in_fd, &offset, file_size);
```

### 4. Memory Mapping cho Large Files

```c
// Better cho files lớn
void *mapped = mmap(NULL, file_size, PROT_READ, 
                    MAP_PRIVATE, fd, 0);
// Access như array
```

### 5. Connection Pooling

```c
// Giữ connections thay vì đóng/mở liên tục
// Reuse sockets khi có thể
```

## Security Considerations

### 1. Input Validation

```c
// Validate all user input
if (port < 1 || port > 65535) {
    fprintf(stderr, "Invalid port\n");
    return -1;
}
```

### 2. Privilege Dropping

```c
// Drop privileges sau khi bind privileged port
if (getuid() == 0) {
    setgid(nobody_gid);
    setuid(nobody_uid);
}
```

### 3. Chroot Jail

```c
// Restrict filesystem access
chroot("/var/jail");
chdir("/");
```

### 4. Resource Limits

```c
struct rlimit limit;
limit.rlim_cur = 1024;  // Soft limit
limit.rlim_max = 2048;  // Hard limit
setrlimit(RLIMIT_NOFILE, &limit);
```

## Lưu Ý Quan Trọng

### General Guidelines

- ✅ Luôn kiểm tra return value của system calls
- ✅ Xử lý errors đúng cách với errno và perror()
- ✅ Đóng file descriptors sau khi dùng xong
- ✅ Tránh buffer overflow với strncpy(), snprintf()
- ✅ Giải phóng bộ nhớ đã cấp phát (malloc/free)
- ✅ Xử lý signals đúng cách (async-signal-safe)
- ✅ Test với nhiều edge cases và error conditions

### Platform Specific

- ⚠️ `epoll()` chỉ có trên Linux (dùng kqueue trên BSD/macOS)
- ⚠️ `inotify` chỉ có trên Linux (dùng kqueue/FSEvents trên macOS)
- ⚠️ Một số flags có thể khác nhau giữa các hệ thống

### Security

- 🔒 Validate tất cả input từ user
- 🔒 Drop privileges khi không cần root
- 🔒 Set resource limits để tránh DoS
- 🔒 Không log sensitive data
- 🔒 Sử dụng secure functions (strncpy thay vì strcpy)

### Performance

- ⚡ Minimize số lượng system calls
- ⚡ Use buffering cho I/O operations
- ⚡ Prefer epoll cho high-concurrency servers
- ⚡ Avoid blocking operations trong event loop
- ⚡ Profile trước khi optimize

### Debugging

- 🐛 Compile với `-g` flag cho debug symbols
- 🐛 Use valgrind để detect memory leaks
- 🐛 Use strace để trace system calls
- 🐛 Enable core dumps: `ulimit -c unlimited`
- 🐛 Test với AddressSanitizer: `-fsanitize=address`

## Quick Reference

### Socket Programming Flow

#### TCP Server

```
socket() -> bind() -> listen() -> accept() -> recv()/send() -> close()
```

#### TCP Client

```
socket() -> connect() -> send()/recv() -> close()
```

#### UDP Server

```
socket() -> bind() -> recvfrom()/sendto() -> close()
```

#### UDP Client

```
socket() -> sendto()/recvfrom() -> close()
```

### Process Management Flow

#### Fork Pattern

```c
pid_t pid = fork();
if (pid < 0) {
    // Error
} else if (pid == 0) {
    // Child process
    exit(0);
} else {
    // Parent process
    wait(NULL);
}
```

#### Exec Pattern

```c
pid_t pid = fork();
if (pid == 0) {
    execl("/bin/ls", "ls", "-l", NULL);
    perror("exec");  // Only if exec fails
    exit(1);
}
wait(NULL);
```

#### Daemon Pattern

```c
fork() -> setsid() -> fork() -> chdir("/") -> umask(0) 
-> close(STDIN/STDOUT/STDERR) -> open("/dev/null")
```

### Signal Handling Pattern

```c
struct sigaction sa;
memset(&sa, 0, sizeof(sa));
sa.sa_handler = handler;
sigemptyset(&sa.sa_mask);
sa.sa_flags = SA_RESTART;
sigaction(SIGINT, &sa, NULL);
```

### File I/O Pattern

```c
int fd = open(path, O_RDWR | O_CREAT, 0644);
if (fd < 0) {
    perror("open");
    return -1;
}

ssize_t n = write(fd, buf, len);
if (n < 0) {
    perror("write");
    close(fd);
    return -1;
}

close(fd);
```

## Appendix

### Exit Codes

```
0   - Success
1   - General error
2   - Misuse of shell command
126 - Command cannot execute
127 - Command not found
128 - Invalid exit argument
130 - Terminated by Ctrl+C (SIGINT)
143 - Terminated by SIGTERM
```

### Common Signals

```
SIGHUP  (1)  - Hangup
SIGINT  (2)  - Interrupt (Ctrl+C)
SIGQUIT (3)  - Quit (Ctrl+\)
SIGILL  (4)  - Illegal instruction
SIGABRT (6)  - Abort
SIGFPE  (8)  - Floating point exception
SIGKILL (9)  - Kill (cannot be caught)
SIGSEGV (11) - Segmentation fault
SIGPIPE (13) - Broken pipe
SIGALRM (14) - Alarm clock
SIGTERM (15) - Termination
SIGUSR1 (10) - User-defined 1
SIGUSR2 (12) - User-defined 2
SIGCHLD (17) - Child process status changed
```

### Socket Options

```c
// Reuse address
int opt = 1;
setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

// Keep alive
setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));

// Timeout
struct timeval tv = {.tv_sec = 5, .tv_usec = 0};
setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

// Buffer size
int bufsize = 8192;
setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
```

### File Open Flags

```c
O_RDONLY    - Read only
O_WRONLY    - Write only
O_RDWR      - Read and write
O_CREAT     - Create if not exists
O_TRUNC     - Truncate to zero length
O_APPEND    - Append mode
O_EXCL      - Fail if file exists (với O_CREAT)
O_NONBLOCK  - Non-blocking mode
O_SYNC      - Synchronous writes
```

### Useful macros

```c
// Wait status macros
WIFEXITED(status)    - True if exited normally
WEXITSTATUS(status)  - Exit code
WIFSIGNALED(status)  - True if killed by signal
WTERMSIG(status)     - Signal number

// Max/Min
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

// Array size
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
```

## Học Tiếp

Sau khi hoàn thành phần này, bạn có thể:

1. ⏭️ **Phần 3**: Kernel Module Development
2. ⏭️ **Phần 4**: SMP Programming với threads nâng cao
3. 📚 Đọc Linux Kernel source code
4. 🔨 Contribute to open source projects
5. 🎯 Xây dựng production-ready applications

## Liên Hệ & Đóng Góp

Nếu tìm thấy lỗi hoặc muốn đóng góp:

- 🐛 Report issues
- 💡 Suggest improvements  
- 🔧 Submit pull requests
- 📖 Improve documentation

---

**Happy Coding!** 🚀

Chúc bạn học tập hiệu quả với System Programming!
