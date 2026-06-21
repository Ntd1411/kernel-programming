# Semaphore - Ví dụ 11

## Tổng quan

Ví dụ này minh họa cơ chế **semaphore** (đèn hiệu) trong lập trình đồng bộ kernel Linux. Semaphore là nguyên thủy đồng bộ dạng đếm, cho phép nhiều thread truy cập tài nguyên đồng thời (khác với mutex chỉ cho phép 1 thread).

## Khái niệm chính

### Semaphore vs Mutex

| Đặc điểm | Mutex | Semaphore |
|----------|-------|-----------|
| Giá trị | Binary (0 hoặc 1) | Counting (0 đến N) |
| Số thread đồng thời | 1 | N |
| Sử dụng | Bảo vệ critical section | Quản lý resource pool |
| Owner | Có (thread lock phải unlock) | Không (bất kỳ thread nào cũng up) |

### Các thao tác

1. **down() / P() / acquire**: Giảm bộ đếm, block nếu = 0
2. **up() / V() / release**: Tăng bộ đếm, đánh thức waiter
3. **down_trylock()**: Thử acquire không block

### Cấu trúc

```c
struct semaphore {
    atomic_int count;          // Bộ đếm (số resource còn lại)
    pthread_mutex_t wait_lock; // Bảo vệ wait list
    pthread_cond_t wait_cond;  // Condition variable cho waiter
};
```

## Demo 1: Printer Pool

Mô phỏng 3 máy in được chia sẻ bởi 6 threads:

```
Semaphore count = 3  (3 máy in khả dụng)

Thread 1, 2, 3: down() thành công (count: 3->2->1->0)
Thread 4, 5, 6: down() block (count < 0, phải đợi)

Thread 1 done: up() (count: 0->1, đánh thức Thread 4)
Thread 4: tiếp tục, lấy máy in
```

**Kết quả mong đợi:**
- Tối đa 3 thread đang in cùng lúc
- Các thread khác đợi đến khi có máy in available

## Demo 2: Producer-Consumer

Mô phỏng bounded buffer với 2 producer và 2 consumer:

```c
semaphore empty_slots = 5;  // Ban đầu buffer rỗng (5 slot trống)
semaphore filled_slots = 0; // Không có item nào

Producer:
    down(&empty_slots);      // Đợi slot trống
    // Thêm item vào buffer
    up(&filled_slots);       // Signal có item mới

Consumer:
    down(&filled_slots);     // Đợi có item
    // Lấy item từ buffer
    up(&empty_slots);        // Signal slot trống
```

**Tính năng:**
- Producer block khi buffer đầy
- Consumer block khi buffer rỗng
- Tự động cân bằng giữa producer/consumer

## Thuật toán

### down() - Fast path và Slow path

```c
// Fast path: count > 0
if (atomic_compare_exchange(&count, &old, old - 1)) {
    return;  // Thành công không block
}

// Slow path: count <= 0
atomic_fetch_sub(&count, 1);  // count có thể âm
while (count < 0) {
    pthread_cond_wait();       // Đợi đến khi count >= 0
}
```

### up() - Đánh thức waiter

```c
atomic_fetch_add(&count, 1);  // Tăng count
pthread_cond_signal();         // Đánh thức 1 waiter
```

## Biên dịch và chạy

```bash
# Biên dịch
make

# Chạy demo
make test

# Hoặc chạy trực tiếp
./semaphore

# Dọn dẹp
make clean
```

## Kết quả mẫu

```
=== Semaphore Demo ===

Demo 1: Printer Pool (3 printers, 6 threads)
==============================================

[Thread 1] Job 1: Requesting printer...
[Thread 2] Job 1: Requesting printer...
[Thread 3] Job 1: Requesting printer...
[Thread 1] Job 1: Got printer! (Active: 1/3, Job#1)
[Thread 2] Job 1: Got printer! (Active: 2/3, Job#2)
[Thread 3] Job 1: Got printer! (Active: 3/3, Job#3)
[Thread 4] Job 1: Requesting printer...
[Thread 5] Job 1: Requesting printer...
[Thread 6] Job 1: Requesting printer...
[Thread 1] Job 1: Printing done, releasing printer
[Thread 4] Job 1: Got printer! (Active: 3/3, Job#4)
...

--- Printer Pool Results ---
Total jobs completed: 12
Fast path acquisitions: 3
Slow path (blocked): 9
Total releases: 12

Demo 2: Producer-Consumer (Buffer size: 5)
==============================================

[Producer 1] Produced item 0 at index 0
[Producer 2] Produced item 100 at index 1
[Consumer 1] Consumed item 0 from index 0
[Consumer 2] Consumed item 100 from index 1
...

✓ Semaphore operations completed successfully
```

## Thống kê

- **Fast path acquisitions**: Số lần down() thành công ngay (count > 0)
- **Slow path (blocked)**: Số lần down() phải block (count = 0)
- **Total releases**: Số lần up() được gọi

## So sánh với Kernel Linux

Kernel Linux có 2 loại semaphore:

1. **struct semaphore**: Counting semaphore (như example này)
   - File: `kernel/locking/semaphore.c`
   - Operations: `down()`, `up()`, `down_interruptible()`, `down_trylock()`

2. **struct rw_semaphore**: Reader-writer semaphore
   - File: `kernel/locking/rwsem.c`
   - Cho phép nhiều reader hoặc 1 writer

## Use cases trong kernel

1. **Resource limiting**: Giới hạn số thread truy cập tài nguyên
2. **Producer-consumer**: Quản lý bounded buffer
3. **Rate limiting**: Giới hạn tốc độ request
4. **Connection pool**: Quản lý số lượng connection

## Ưu điểm

- ✅ Cho phép nhiều thread đồng thời
- ✅ Linh hoạt với bộ đếm
- ✅ Có thể dùng cho signaling giữa threads

## Nhược điểm

- ❌ Không có owner (bất kỳ thread nào cũng up được)
- ❌ Có thể gây deadlock nếu dùng sai
- ❌ Phức tạp hơn mutex

## Tham khảo

- Linux Kernel: `include/linux/semaphore.h`
- Linux Kernel: `kernel/locking/semaphore.c`
- LWN Article: "Semaphores in the kernel"
