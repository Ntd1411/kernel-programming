# Per-CPU Data - Ví dụ 12

## Tổng quan

Ví dụ này minh họa kỹ thuật tối ưu hóa **per-CPU data** (dữ liệu riêng từng CPU) trong kernel Linux. Mỗi CPU có bản sao riêng của dữ liệu, tránh cache line bouncing và contention, cải thiện hiệu năng đáng kể trên hệ thống đa lõi.

## Khái niệm chính

### Per-CPU Data vs Shared Data

| Đặc điểm | Shared Data | Per-CPU Data |
|----------|-------------|--------------|
| Số bản sao | 1 (chia sẻ giữa tất cả CPU) | N (mỗi CPU 1 bản) |
| Đồng bộ | Cần atomic/lock | Không cần (local access) |
| Cache behavior | Cache line bouncing | Cache friendly |
| Performance | Chậm (contention) | Nhanh (no contention) |
| Memory usage | Ít (1 copy) | Nhiều (N copies) |

### Tại sao cần Per-CPU Data?

**Vấn đề với Shared Counter:**
```
CPU0: atomic_add(&counter)  -> Cache line in MODIFIED state on CPU0
CPU1: atomic_add(&counter)  -> Invalidate CPU0's cache, fetch to CPU1
CPU0: atomic_add(&counter)  -> Invalidate CPU1's cache, fetch to CPU0
                            -> "Cache ping-pong" / bouncing
```

**Giải pháp với Per-CPU Counter:**
```
CPU0: counter[0]++  -> CPU0's local cache, no contention
CPU1: counter[1]++  -> CPU1's local cache, no contention
CPU2: counter[2]++  -> CPU2's local cache, no contention
                    -> Mỗi CPU làm việc trên cache line riêng
```

### Kiến trúc Per-CPU

```c
// Cấu trúc per-CPU
typedef struct {
    long counter;
    long operations;
    char padding[64 - 2*sizeof(long)]; // Pad đến cache line (64 bytes)
} percpu_data_t;

// Mảng per-CPU: 1 entry cho mỗi CPU
percpu_data_t percpu_counters[MAX_CPUS] __attribute__((aligned(64)));
```

**Cache Line Alignment:**
- Cache line size: thường 64 bytes trên x86
- Alignment đảm bảo mỗi CPU struct nằm trên cache line riêng
- Tránh false sharing giữa các CPU

## API Cơ bản

### Truy cập Per-CPU Data

```c
// 1. Get CPU ID và disable preemption
int cpu = get_cpu();

// 2. Truy cập data của CPU hiện tại
percpu_data_t *ptr = per_cpu_ptr(percpu_counters, cpu);
ptr->counter++;

// 3. Re-enable preemption
put_cpu();
```

### Aggregation

```c
// Tính tổng từ tất cả CPU
long total = 0;
for (int cpu = 0; cpu < MAX_CPUS; cpu++) {
    total += percpu_counters[cpu].counter;
}
```

## Demo: So sánh Performance

Demo chạy 2 test với 8 threads, mỗi thread thực hiện 1 triệu operations:

### Test 1: Per-CPU Counters
- Mỗi thread pin vào 1 CPU cụ thể
- Thread increment counter của CPU của nó
- Không có contention, không cần atomic operations

### Test 2: Shared Counter
- Tất cả threads increment cùng 1 counter
- Sử dụng atomic operations
- Có contention, cache line bouncing

## Biên dịch và chạy

```bash
# Biên dịch
make

# Chạy demo
make test

# Hoặc chạy trực tiếp
./per_cpu_data

# Dọn dẹp
make clean
```

## Kết quả mẫu

```
=== Per-CPU Data Demo ===

Configuration:
  Threads: 8
  CPUs available: 8
  Iterations per thread: 1000000
  Total operations: 8000000

Test 1: Per-CPU Counters (No Contention)
==========================================
Total time: 45.23 ms
Average per thread: 5.65 ms
Final counter sum: 8000000
Total operations: 8000000

Per-CPU distribution:
  CPU 0: counter=1000000, ops=1000000
  CPU 1: counter=1000000, ops=1000000
  CPU 2: counter=1000000, ops=1000000
  CPU 3: counter=1000000, ops=1000000
  CPU 4: counter=1000000, ops=1000000
  CPU 5: counter=1000000, ops=1000000
  CPU 6: counter=1000000, ops=1000000
  CPU 7: counter=1000000, ops=1000000


Test 2: Shared Counter (With Contention)
=========================================
Total time: 312.45 ms
Average per thread: 39.06 ms
Final counter: 8000000
Total operations: 8000000


=== Performance Comparison ===
Per-CPU: 45.23 ms
Shared:  312.45 ms
Speedup: 6.91x faster with per-CPU

✓ Per-CPU optimization successful!
  Benefit: 591% reduction in execution time
```

## Phân tích Performance

**Speedup factors:**
- 2 CPUs: ~1.5-2x faster
- 4 CPUs: ~3-4x faster
- 8 CPUs: ~6-8x faster
- 16+ CPUs: ~10-15x faster

**Lý do:**
- Không có cache line bouncing
- Không cần atomic instructions (chậm hơn regular instructions)
- Mỗi CPU làm việc độc lập trên cache line riêng

## So sánh với Kernel Linux

### Per-CPU API trong kernel

```c
// Khai báo per-CPU variable
DEFINE_PER_CPU(long, counter);

// Truy cập
int cpu = get_cpu();
long *ptr = per_cpu_ptr(&counter, cpu);
(*ptr)++;
put_cpu();

// Hoặc dùng macro
this_cpu_inc(counter);  // Atomic cho CPU hiện tại
```

### Các file liên quan

- `include/linux/percpu.h` - Per-CPU API definitions
- `mm/percpu.c` - Per-CPU allocator
- `include/linux/percpu-defs.h` - Per-CPU macros

## Use cases trong kernel

1. **Statistics counters**: Network packets, file operations, etc.
2. **Per-CPU caches**: Memory allocator, buffer caches
3. **Softirq processing**: Per-CPU softirq vectors
4. **RCU implementation**: Per-CPU read-side counters
5. **Scheduler**: Per-CPU run queues

## Ưu điểm

- ✅ Performance cao (no contention)
- ✅ Cache friendly (no bouncing)
- ✅ Scalable trên nhiều CPU
- ✅ Không cần synchronization cho local access

## Nhược điểm

- ❌ Tốn nhiều memory hơn (N copies)
- ❌ Aggregation có overhead
- ❌ Cần disable preemption khi access
- ❌ Không phù hợp cho data cần consistency nghiêm ngặt

## Khi nào dùng Per-CPU?

**Nên dùng:**
- Counters và statistics
- Temporary buffers
- Read-mostly data với rare updates
- High-frequency operations

**Không nên dùng:**
- Data cần strict consistency
- Data có size lớn
- Ít operations (overhead không đáng kể)

## Tham khảo

- Linux Kernel: `include/linux/percpu.h`
- Linux Kernel: `Documentation/core-api/per-cpu.rst`
- LWN Article: "Per-CPU variables in the kernel"
