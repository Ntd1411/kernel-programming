# Phần 4: Symmetric Multi-Processing (SMP)

Lập trình đa luồng và tận dụng hiệu năng đa nhân CPU trên Linux.

## Mục Tiêu

- Hiểu kiến trúc SMP và đa nhân
- Lập trình đa luồng với POSIX Threads (pthreads)
- Đồng bộ hóa: mutex, semaphore, condition variables
- CPU affinity và scheduling
- Xử lý race conditions và deadlock
- Tối ưu hiệu năng đa luồng

## Cấu Trúc

### 1. Pthread Basics (pthread-basics/)

Lập trình đa luồng cơ bản:
- `thread_create.c` - Tạo và kết thúc thread
- `thread_args.c` - Truyền tham số cho thread
- `thread_return.c` - Nhận giá trị trả về từ thread
- `thread_detach.c` - Detached threads
- `thread_cancel.c` - Hủy thread

### 2. Synchronization (synchronization/)

Đồng bộ hóa thread:
- `mutex_basic.c` - Mutex locks
- `mutex_deadlock.c` - Phát hiện và tránh deadlock
- `rwlock.c` - Reader-Writer locks
- `semaphore.c` - Semaphore
- `condition_var.c` - Condition variables
- `barrier.c` - Thread barriers
- `spinlock.c` - Spinlocks

### 3. CPU Affinity (cpu-affinity/)

CPU affinity và scheduling:
- `cpu_info.c` - Hiển thị thông tin CPU
- `set_affinity.c` - Gán thread vào CPU cụ thể
- `load_balance.c` - Cân bằng tải giữa các CPU
- `numa_aware.c` - NUMA-aware programming
- `scheduling_policy.c` - Real-time scheduling

## Yêu Cầu

```bash
# Cài đặt pthread library (thường đã có sẵn)
sudo apt install libc6-dev

# Cài đặt numactl để test NUMA
sudo apt install numactl

# Kiểm tra số lượng CPU
nproc
lscpu
```

## Biên Dịch

```bash
# Compile với pthread
gcc -o program program.c -pthread

# Compile với tối ưu
gcc -O2 -o program program.c -pthread

# Compile tất cả
make all
```

## Cấu Trúc Thread Cơ Bản

```c
#include <pthread.h>
#include <stdio.h>

void *thread_function(void *arg) {
    // Thread code here
    printf("Thread running\n");
    return NULL;
}

int main() {
    pthread_t thread;
    
    // Tạo thread
    pthread_create(&thread, NULL, thread_function, NULL);
    
    // Đợi thread kết thúc
    pthread_join(thread, NULL);
    
    return 0;
}
```

## Đồng Bộ Hóa

### Mutex (Mutual Exclusion)

```c
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_lock(&mutex);
// Critical section
pthread_mutex_unlock(&mutex);
```

### Semaphore

```c
#include <semaphore.h>

sem_t semaphore;
sem_init(&semaphore, 0, 1);

sem_wait(&semaphore);
// Critical section
sem_post(&semaphore);
```

### Condition Variable

```c
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Wait
pthread_mutex_lock(&mutex);
pthread_cond_wait(&cond, &mutex);
pthread_mutex_unlock(&mutex);

// Signal
pthread_cond_signal(&cond);
```

## CPU Affinity

```c
#define _GNU_SOURCE
#include <sched.h>

cpu_set_t cpuset;
CPU_ZERO(&cpuset);
CPU_SET(0, &cpuset);  // Gán vào CPU 0

pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
```

## Các Vấn Đề Thường Gặp

### 1. Race Condition
Nhiều thread truy cập dữ liệu chung mà không đồng bộ hóa.

**Giải pháp:** Sử dụng mutex, semaphore hoặc atomic operations.

### 2. Deadlock
Hai hoặc nhiều thread đợi lẫn nhau.

**Giải pháp:** 
- Luôn lock theo thứ tự nhất định
- Sử dụng trylock
- Timeout

### 3. False Sharing
Nhiều thread truy cập dữ liệu trên cùng cache line.

**Giải pháp:** Padding để tách cache lines.

### 4. Thread Starvation
Thread không được cấp phát CPU.

**Giải pháp:** Điều chỉnh priority và scheduling policy.

## Tối Ưu Hiệu Năng

1. **Giảm contention:** Ít lock hơn, lock ngắn hơn
2. **Locality:** Dữ liệu thread-local, NUMA awareness
3. **Load balancing:** Phân phối công việc đều
4. **Lock-free algorithms:** Atomic operations
5. **Thread pool:** Tái sử dụng threads

## Công Cụ Debug

```bash
# Kiểm tra thread đang chạy
ps -eLf | grep program_name

# Monitor CPU usage
top -H
htop

# Valgrind helgrind (race condition)
valgrind --tool=helgrind ./program

# Valgrind DRD (thread errors)
valgrind --tool=drd ./program

# perf (performance analysis)
perf record ./program
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
