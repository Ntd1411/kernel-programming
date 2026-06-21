# Lập Trình SMP - Các Ví Dụ Đồng Bộ Hóa Kernel

Bộ sưu tập 12 ví dụ thực hành minh họa các cơ chế đồng bộ hóa trong Linux kernel cho hệ thống Symmetric Multi-Processing (SMP).

## Giới Thiệu

Thư mục này chứa các ví dụ user-space mô phỏng các primitive đồng bộ hóa của kernel, giúp hiểu sâu về:
- Race conditions và cách phát hiện
- Atomic operations và memory ordering
- Spin locks (naive và optimized)
- Mutexes (fast path, slow path, unlock)
- Interrupt disabling và preemption control
- RCU (Read-Copy-Update)

Mỗi ví dụ được thiết kế để minh họa một khái niệm cụ thể từ lý thuyết SMP, dựa trên tài liệu `smp-lecture.html` và kế hoạch trong `smp-audit.html`.

## Danh Sách Các Ví Dụ

### [ex01: Race Condition](./race-condition-ex01/)
**Khái niệm:** Race condition cổ điển trong quản lý tài nguyên  
**Demo:** Lỗi double-free khi nhiều luồng đồng thời giảm counter  
**File:** [race-condition-ex01/README.md](./race-condition-ex01/README.md)

### [ex02: Atomic Fix](./atomic-fix-ex02/)
**Khái niệm:** Sửa race condition bằng atomic operations  
**Demo:** Sử dụng `atomic_fetch_sub` để đảm bảo test-and-decrement atomic  
**File:** [atomic-fix-ex02/README.md](./atomic-fix-ex02/README.md)

### [ex03: IRQ Disable](./irq-disable-ex03/)
**Khái niệm:** Tắt ngắt để đồng bộ hóa  
**Demo:** Mô phỏng `local_irq_disable/enable/save/restore` bằng signal masking  
**File:** [irq-disable-ex03/README.md](./irq-disable-ex03/README.md)

### [ex04: Spinlock Basic](./spinlock-basic-ex04/)
**Khái niệm:** Spin lock cơ bản với atomic test-and-set  
**Demo:** Mô phỏng lệnh `lock bts` của x86, theo dõi contention  
**File:** [spinlock-basic-ex04/README.md](./spinlock-basic-ex04/README.md)

### [ex05: Spinlock Optimized](./spinlock-optimized-ex05/)
**Khái niệm:** Tối ưu spin lock để giảm cache thrashing  
**Demo:** Read-first optimization và lệnh PAUSE, so sánh với naive version  
**File:** [spinlock-optimized-ex05/README.md](./spinlock-optimized-ex05/README.md)

### [ex06: Preemption Counter](./preemption-counter-ex06/)
**Khái niệm:** Cơ chế preemption counter của kernel  
**Demo:** Bit-field layout cho PREEMPT/SOFTIRQ/HARDIRQ/NMI, hàm `in_interrupt()`  
**File:** [preemption-counter-ex06/README.md](./preemption-counter-ex06/README.md)

### [ex07: Mutex Lock Fast Path](./mutex-lock-ex07/)
**Khái niệm:** Fast path của mutex_lock (optimistic acquisition)  
**Demo:** Atomic compare-exchange với acquire semantics  
**File:** [mutex-lock-ex07/README.md](./mutex-lock-ex07/README.md)

### [ex08: Mutex Lock Slow Path](./mutex-lock-slow-ex08/)
**Khái niệm:** Slow path của mutex_lock (sleeping và wait queue)  
**Demo:** Quản lý wait queue, luồng sleep và được đánh thức  
**File:** [mutex-lock-slow-ex08/README.md](./mutex-lock-slow-ex08/README.md)

### [ex09: Mutex Unlock](./mutex-unlock-ex09/)
**Khái niệm:** Unlock mutex (fast và slow paths)  
**Demo:** Fast path atomic clear, slow path wake waiters, WAITERS flag  
**File:** [mutex-unlock-ex09/README.md](./mutex-unlock-ex09/README.md)

### [ex10: Memory Ordering và RCU](./memory-ordering-rcu-ex10/)
**Khái niệm:** Memory barriers và RCU synchronization  
**Demo:** Compiler barriers, memory barriers (rmb/wmb/mb), RCU list operations  
**File:** [memory-ordering-rcu-ex10/README.md](./memory-ordering-rcu-ex10/README.md)

### [ex11: Semaphore](./semaphore-ex11/)
**Khái niệm:** Counting semaphore cho phép N threads truy cập đồng thời  
**Demo:** Printer pool (3 printers cho 6 threads) và producer-consumer  
**File:** [semaphore-ex11/README.md](./semaphore-ex11/README.md)

### [ex12: Per-CPU Data](./per-cpu-data-ex12/)
**Khái niệm:** Tối ưu hóa per-CPU để tránh cache line bouncing  
**Demo:** So sánh performance: per-CPU counters vs shared counter (6-8x speedup)  
**File:** [per-cpu-data-ex12/README.md](./per-cpu-data-ex12/README.md)

## Tài Liệu Tham Khảo

- **Lý thuyết:** `smp-lecture.html` - Bài giảng đầy đủ về SMP synchronization
- **Kế hoạch:** `smp-audit.html` - Audit plan với 10 ví dụ cơ bản (VD 01-10)
- **Mở rộng:** ex11-ex12 bổ sung semaphore và per-CPU data

Mỗi ví dụ có phần "Tài Liệu Tham Khảo" trỏ đến phần tương ứng trong bài giảng.

## Cấu Trúc Mỗi Ví Dụ

Mỗi thư mục ví dụ chứa đúng 3 files:

```
example-name-exXX/
├── example_name.c    # Mã nguồn C minh họa khái niệm
├── Makefile          # Build script (make, make clean, make test)
└── README.md         # Giải thích chi tiết (Tiếng Việt)
```

## Biên Dịch và Chạy

### Biên Dịch Một Ví Dụ

```bash
cd race-condition-ex01/
make                  # Biên dịch
./race_condition      # Chạy demo
make clean            # Dọn dẹp
```

### Biên Dịch Tất Cả (từ thư mục smp-programming/)

```bash
# Biên dịch tất cả 12 ví dụ
for dir in *-ex*/; do
    cd "$dir"
    make
    cd ..
done

# Chạy tất cả
for dir in *-ex*/; do
    cd "$dir"
    echo "=== Running $(basename $dir) ==="
    make test 2>/dev/null || ./*[!.]  # Chạy executable
    cd ..
done
```

## Lộ Trình Học Tập Đề Xuất

### Nhóm 1: Cơ Bản (ex01-ex02)
1. **ex01** - Hiểu race condition là gì
2. **ex02** - Học cách sửa bằng atomic operations

### Nhóm 2: Locks (ex03-ex05)
3. **ex03** - Interrupt disabling (đồng bộ đơn giản)
4. **ex04** - Spin lock cơ bản (busy-wait)
5. **ex05** - Spin lock tối ưu (giảm cache thrashing)

### Nhóm 3: Nâng Cao (ex06-ex10)
6. **ex06** - Preemption counter (theo dõi context)
7. **ex07** - Mutex fast path (acquire nhanh)
8. **ex08** - Mutex slow path (sleep khi contention)
9. **ex09** - Mutex unlock (wake waiters)
10. **ex10** - Memory ordering và RCU (lock-free)

### Nhóm 4: Mở Rộng (ex11-ex12)
11. **ex11** - Semaphore (counting, resource pools)
12. **ex12** - Per-CPU data (tối ưu cache, no contention)

## Yêu Cầu Hệ Thống

- **Compiler:** GCC với hỗ trợ C11 (`-std=c11`)
- **Libraries:** pthread (libc6-dev)
- **Platform:** Linux (x86-64 recommended)

```bash
# Kiểm tra GCC
gcc --version

# Cài đặt dependencies (nếu cần)
sudo apt install build-essential
```

## Ghi Chú

- Tất cả code là **user-space simulation** của các kernel primitives
- Không thể tắt ngắt thật trong user-space → dùng signal masking
- Không có kernel scheduler → dùng pthread và sleep/yield
- Mục đích: **Hiểu concepts**, không phải triển khai production

## Lỗi Thường Gặp

**Segmentation fault:** Kiểm tra race condition hoặc double-free (đặc biệt ex01)  
**Deadlock:** Xem ex08, đảm bảo wait queue được quản lý đúng  
**Counter không khớp:** Contention cao, kiểm tra atomic operations

## Tác Giả & Nguồn

Các ví dụ được tạo dựa trên:
- Linux kernel source code (`kernel/locking/`)
- Documentation: `smp-lecture.html` và `smp-audit.html`
- Kiến trúc: x86-64 assembly và atomic instructions

---

**Bắt đầu từ [ex01: Race Condition](./race-condition-ex01/)** để thấy vấn đề, sau đó học cách sửa qua các ví dụ tiếp theo!
perf report
```

## Bài Tập

1. **Producer-Consumer:** Implement queue với nhiều producers và consumers
2. **Parallel Sort:** Sắp xếp mảng lớn sử dụng multiple threads
3. **Matrix Multiplication:** Nhân ma trận song song
4. **Web Server:** Multithreaded web server với thread pool
5. **Benchmark:** So sánh hiệu năng single-thread vs multi-thread

## Tài Liệu Tham Khảo

- POSIX Threads Programming: `man pthread`
- Programming with POSIX Threads (Butenhof)
- The Art of Multiprocessor Programming
- Linux System Programming (Robert Love)
- Intel Threading Building Blocks

## Lưu Ý An Toàn

- Luôn kiểm tra return value của pthread functions
- Cleanup: pthread_join hoặc pthread_detach
- Tránh shared state khi có thể
- Sử dụng thread-safe functions
- Cẩn thận với signal trong multithreaded programs

## Performance Metrics

```bash
# Đo thời gian thực thi
time ./program

# CPU utilization
mpstat 1

# Context switches
pidstat -w 1 -p PID
```

---

**Chú ý:** Lập trình đa luồng phức tạp và dễ gặp lỗi. Luôn test kỹ càng với các công cụ như Valgrind, ThreadSanitizer, và stress testing.
