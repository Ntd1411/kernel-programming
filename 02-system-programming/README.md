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
- `fork_example.c` - Tạo tiến trình con
- `exec_family.c` - Thực thi chương trình khác
- `signal_handler.c` - Xử lý signals
- `zombie_reaper.c` - Xử lý zombie process
- `daemon.c` - Tạo daemon process

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

### Single file:

```bash
gcc -o program program.c
gcc -o program program.c -lpthread  # Nếu dùng threads
```

### Với Makefile:

```bash
cd process-management/
make
./fork_example
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

### Simple TCP Server:

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

## Bài Tập

1. Viết chương trình tạo nhiều tiến trình con xử lý song song
2. Tạo chat server/client đơn giản bằng socket
3. Xây dựng file manager với các thao tác cơ bản
4. Viết HTTP server đơn giản xử lý GET request
5. Tạo chương trình giám sát thay đổi file trong thư mục

## Debug và Test

```bash
# Compile với debug symbols
gcc -g -o program program.c

# Debug với gdb
gdb ./program

# Check memory leaks
valgrind --leak-check=full ./program

# Trace system calls
strace ./program
```

## Tài Liệu Tham Khảo

- Man pages: `man 2 <syscall>`
- Beej's Guide to Network Programming
- The Linux Programming Interface
- Advanced Programming in the UNIX Environment

## Lưu Ý Quan Trọng

- Luôn kiểm tra return value của system calls
- Xử lý errors đúng cách (errno, perror)
- Đóng file descriptors sau khi dùng xong
- Tránh buffer overflow
- Giải phóng bộ nhớ đã cấp phát
- Xử lý signals đúng cách
- Test với nhiều edge cases
