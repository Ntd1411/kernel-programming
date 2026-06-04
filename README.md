# Bài Tập Lớn - Lập Trình Hệ Thống Linux

Dự án bài tập lớn về lập trình hệ thống trên Ubuntu/Linux bao gồm 4 phần chính.

## Cấu Trúc Thư Mục

```
kernel-linux/
├── 01-shell-scripting/          # Lập trình Shell
│   ├── file-management/         # Quản lý file
│   ├── task-scheduler/          # Lập lịch tác vụ
│   ├── time-management/         # Quản lý thời gian hệ thống
│   └── package-management/      # Cài đặt/gỡ bỏ chương trình
│
├── 02-system-programming/       # Lập trình hệ thống C/C++
│   ├── process-management/      # Quản lý tiến trình
│   ├── file-io/                 # Quản lý file I/O
│   ├── socket-programming/      # Lập trình socket
│   └── network-programming/     # Lập trình mạng
│
├── 03-kernel-module/            # Module nhân Linux
│   ├── basic-module/            # Module cơ bản
│   ├── char-device/             # Character device driver
│   └── proc-interface/          # Proc filesystem interface
│
├── 04-smp-programming/          # Symmetric Multi-Processing
│   ├── pthread-basics/          # Lập trình đa luồng cơ bản
│   ├── synchronization/         # Đồng bộ hóa
│   └── cpu-affinity/            # CPU affinity và scheduling
│
├── docs/                        # Tài liệu
├── tests/                       # Test scripts
└── logs/                        # Log files
```

## Yêu Cầu Hệ Thống

- Ubuntu 20.04 hoặc mới hơn
- GCC/G++ compiler
- Linux kernel headers
- Make, Git
- Root/sudo privileges

## Cài Đặt Môi Trường

```bash
# Cài đặt các công cụ cần thiết
sudo apt update
sudo apt install build-essential linux-headers-$(uname -r)
sudo apt install git vim manpages-dev manpages-posix-dev
```

## Phần 1: Lập Trình Shell

Các script bash để quản lý hệ thống tự động.

Xem chi tiết: [01-shell-scripting/README.md](01-shell-scripting/README.md)

## Phần 2: Lập Trình Hệ Thống

Lập trình C/C++ tương tác với kernel thông qua system calls.

Xem chi tiết: [02-system-programming/README.md](02-system-programming/README.md)

## Phần 3: Module Nhân Linux

Xây dựng và tích hợp loadable kernel module (LKM).

Xem chi tiết: [03-kernel-module/README.md](03-kernel-module/README.md)

## Phần 4: SMP Programming

Lập trình đa luồng và tận dụng đa nhân CPU.

Xem chi tiết: [04-smp-programming/README.md](04-smp-programming/README.md)

## Biên Dịch và Chạy

Mỗi thư mục con có Makefile riêng. Để build toàn bộ project:

```bash
make all
```

## Tài Liệu Tham Khảo

- Linux Man Pages: `man 2 <syscall>`
- The Linux Programming Interface (Michael Kerrisk)
- Linux Device Drivers (LDD3)
- Linux Kernel Development (Robert Love)

## Tác Giả

[Tên của bạn]

## Giấy Phép

Educational purpose only.
