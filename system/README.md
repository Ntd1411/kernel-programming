# Lập Trình Hệ Thống (System Programming)

Các ví dụ lập trình C tương tác với hệ thống thông qua system calls trên Linux.

## Giới Thiệu

Thư mục này chứa 4 module chính:
- **File Operations** - Quản lý file, memory mapping, inotify
- **Process Management** - Fork, exec, waitpid, signals
- **Network Programming** - Socket, TCP/UDP, steganography
- **Socket Programming** - Unix sockets và network protocols

Mỗi module minh họa các khái niệm quan trọng trong lập trình hệ thống Linux.

## 1. File Operations (file/)

Quản lý file, directory walks, memory mapping, và inotify.

### Các File

| File | Mô Tả |
|------|-------|
| `file_operations.c` | File I/O: open, read, write, close, lseek |
| `memory_map.c` | Memory mapping với mmap/munmap |
| `directory_walk.c` | Traversing directory trees |
| `file_locking.c` | File locking (fcntl) |
| `inotify_example.c` | File system events monitoring với inotify |
| `vfs_module/` | Virtual File System examples |
| `Makefile` | Build configuration |

### Compile & Run

```bash
cd file
make
./a.out          # Chạy file_operations
./memory_map     # Chạy memory mapping demo
```

### Khái Niệm Chính

- **File Descriptors**: Quản lý file handles
- **System Calls**: open, read, write, close, lseek
- **Memory Mapping**: mmap/munmap, shared memory
- **Directory Operations**: opendir, readdir, closedir
- **File Locking**: fcntl, flock
- **Inotify**: Theo dõi thay đổi file system

## 2. Process Management (process/)

Quản lý tiến trình, fork, exec, signals.

### Các File

| File | Mô Tả |
|------|-------|
| `exec_family.c` | exec system calls (execl, execv, execle, execve) |
| `fork_wait.c` | Fork và wait system calls |
| `signals.c` | Signal handling (SIGCHLD, SIGTERM, etc.) |
| `... | Các file khác |
| `Makefile` | Build configuration |

### Compile & Run

```bash
cd process
make
# Chạy từng demo
```

### Khái Niệm Chính

- **Process Creation**: fork()
- **Process Replacement**: exec family
- **Process Termination**: exit(), wait(), waitpid()
- **Signals**: Signal handlers, signal masking
- **Process Synchronization**: wait queues

## 3. Network Programming (network/)

Socket programming, TCP/UDP, steganography.

### Các File

| File | Mô Tả |
|------|-------|
| `loopback_driver.c` | Loopback network driver |
| `skbuff_demo.c` | Socket buffer (skbuff) demo |
| `tcp_steganography.c` | TCP-based steganography |
| `stego_reader.c` | Reader for steganography data |
| `test_loopback.sh` | Test script for loopback |
| `test_skbuff.sh` | Test script for skbuff |
| `test_steganography.sh` | Test script for steganography |
| `Makefile` | Build configuration |

### Compile & Run

```bash
cd network
make
sudo ./a.out     # Có thể cần sudo cho driver
# Hoặc chạy test scripts
bash test_loopback.sh
```

### Khái Niệm Chính

- **Socket API**: socket, bind, listen, accept, connect
- **Protocol Stacks**: TCP/UDP, packet structures
- **Kernel Buffers**: skbuff, packet buffering
- **Steganography**: Ẩn dữ liệu trong network packets

## 4. Socket Programming (socket/)

Unix sockets và socket APIs.

### Compile & Run

```bash
cd socket
make
./a.out
```

### Khái Niệm Chính

- **Socket Types**: SOCK_STREAM, SOCK_DGRAM
- **Address Families**: AF_UNIX, AF_INET, AF_INET6
- **Socket Operations**: bind, listen, accept, connect
- **Data Transfer**: send, recv, sendto, recvfrom

## Build Toàn Bộ System Module

```bash
# Từ thư mục system/
make all        # Build tất cả modules
make clean      # Clean object files
make distclean  # Clean all generated files
```

## Yêu Cầu Hệ Thống

- GCC compiler
- Linux kernel headers: `linux-headers-$(uname -r)`
- Make
- Standard C library (glibc)

### Cài Đặt trên Ubuntu

```bash
sudo apt update
sudo apt install build-essential linux-headers-$(uname -r)
```

## Tài Liệu Tham Khảo

- Linux Man Pages: `man 2 <syscall>` hoặc `man 3 <libc_function>`
- The Linux Programming Interface (Michael Kerrisk) - Sách tham khảo chính
- Linux Kernel Development (Robert Love)
- Advanced Programming in the UNIX Environment (Stevens & Rago)

Các ví dụ quan trọng:
```bash
man 2 open
man 2 fork
man 2 execve
man 2 socket
man 7 socket
```

## Cấu Trúc Thư Mục

```
system/
├── file/                # File operations module
│   ├── *.c             # Source files
│   ├── vfs_module/     # VFS examples
│   └── Makefile
├── process/            # Process management module
│   ├── *.c
│   └── Makefile
├── network/            # Network programming module
│   ├── *.c
│   ├── *.sh            # Test scripts
│   └── Makefile
├── socket/             # Socket programming module
│   ├── *.c
│   └── Makefile
├── Makefile            # Master makefile
└── README.md           # This file
```

## Chạy Các Demo

### 1. File Operations Demo
```bash
cd system/file
make
./a.out
```

### 2. Process Management Demo
```bash
cd system/process
make
# Thực thi các tiến trình demo
```

### 3. Network Demo (có thể cần sudo)
```bash
cd system/network
make
sudo ./loopback_driver
```

## Học Tập & Thực Hành

1. **Bắt đầu từ file operations** - Hiểu file descriptors
2. **Tiếp đến process management** - Hiểu process lifecycle
3. **Sau đó network programming** - Hiểu socket communication
4. **Cuối cùng kernel concepts** - Kết hợp kiến thức

Mỗi module có README riêng với chi tiết hơn.

## Tác Giả

Kernel Programming Project

## Giấy Phép

Educational purpose only.
