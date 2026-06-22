# Bài Tập Lớn - Lập Trình Hệ Thống Linux

Dự án bài tập lớn về lập trình hệ thống trên Ubuntu/Linux bao gồm 4 phần chính.

## Cấu Trúc Thư Mục

``` folder
kernel-programming/
├── shell-scripting/             # Phần 1: Lập trình Shell
│   ├── file-management/         # Quản lý file & thư mục
│   ├── task-scheduler/          # Lập lịch tác vụ (cron & systemd)
│   ├── time-management/         # Quản lý thời gian hệ thống
│   ├── package-management/      # Cài đặt/gỡ bỏ chương trình
│   ├── demo/                    # Demo scripts
│   ├── gui_launcher.py          # GUI launcher (Python/tkinter)
│   ├── demo.sh                  # Demo menu chính
│   └── quick_test.sh            # Quick test
│
├── smp-programming/             # Phần 2: Symmetric Multi-Processing
│   ├── race-condition-ex01/     # Race condition & atomic operations
│   ├── atomic-fix-ex02/         # Fix với atomic operations
│   ├── irq-disable-ex03/        # IRQ disable & signal masking
│   ├── spinlock-basic-ex04/     # Spinlock cơ bản
│   ├── spinlock-optimized-ex05/ # Spinlock tối ưu
│   ├── preemption-counter-ex06/ # Preemption counter
│   ├── mutex-lock-ex07/         # Mutex lock fast path
│   ├── mutex-lock-slow-ex08/    # Mutex lock slow path
│   ├── mutex-unlock-ex09/       # Mutex unlock
│   ├── memory-ordering-rcu-ex10/# Memory ordering & RCU
│   ├── semaphore-ex11/          # Semaphore
│   ├── per-cpu-data-ex12/       # Per-CPU data
│   ├── demo.sh                  # Demo runner
│   └── doc/                     # Lecture slides & audit
│
├── system/                      # Phần 3: System Programming (C)
│   ├── file/                    # File operations & memory mapping
│   ├── network/                 # Network programming
│   ├── process/                 # Process management
│   └── socket/                  # Socket programming
│
├── DOCUMENTATION_PLAN.md        # Kế hoạch cải thiện tài liệu
└── README.md                    # File này
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

## Phần 1: Lập Trình Shell (shell-scripting/)

Các script bash để quản lý hệ thống tự động trên Ubuntu/Linux.

**Bao gồm:**
- File management (backup, cleanup, find duplicates)
- Task scheduling (cron, systemd timers)
- Time management (time tracker, stopwatch)
- Package management (install, update, dependencies)
- **NEW**: GUI launcher (Python/tkinter) cho Ubuntu 24.10

Xem chi tiết: [shell-scripting/README.md](shell-scripting/README.md)

## Phần 2: SMP Programming (smp-programming/)

12 ví dụ thực hành về các primitive đồng bộ hóa kernel cho hệ thống Symmetric Multi-Processing.

**Bao gồm:**
- Race conditions & atomic operations
- Spin locks (naive & optimized)
- Mutexes (fast path, slow path, unlock)
- RCU (Read-Copy-Update)
- Preemption counter & interrupt handling

Xem chi tiết: [smp-programming/README.md](smp-programming/README.md)

## Phần 3: System Programming (system/)

Lập trình C tương tác với hệ thống thông qua system calls.

**Bao gồm:**
- File operations (open, read, write, mmap)
- Process management (fork, exec, wait)
- Socket programming (TCP/UDP)
- Network I/O (loopback driver, steganography)

Xem chi tiết: [system/README.md](system/README.md)

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

## Quick Start

### 1. Setup Ubuntu 24.10
```bash
sudo apt update
sudo apt install build-essential linux-headers-$(uname -r)
sudo apt install git gcc g++ make python3-tk
```

### 2. Clone & Explore
```bash
git clone <repo-url>
cd kernel-programming
```

### 3. Chạy Shell Scripting GUI (Ubuntu 24.10)
```bash
cd shell-scripting
./setup_gui.sh      # Setup dependencies
./launch_gui.sh     # Launch GUI
```

### 4. Chạy SMP Examples
```bash
cd smp-programming
./demo.sh           # Interactive demo menu
# hoặc
cd race-condition-ex01 && make
```

### 5. Chạy System Programming Examples
```bash
cd system/file
make && ./a.out
```

## Tài Liệu Bổ Sung

- [Documentation Plan](DOCUMENTATION_PLAN.md) - Kế hoạch cải thiện tài liệu
- [Shell Scripting GUI Summary](shell-scripting/GUI_SUMMARY.md) - Chi tiết GUI
- [SMP Lecture](smp-programming/doc/smp-lecture.html) - Bài giảng SMP
- [SMP Audit](smp-programming/doc/smp-audit.html) - Audit smp-programming

## Tác Giả

Kernel Programming Project - Educational

## Giấy Phép

Educational purpose only. For learning Linux kernel concepts.
