# SMP Synchronization Kernel Module

Module Linux minh họa **5 cơ chế đồng bộ hóa** hoàn chỉnh cho hệ thống SMP,
mô phỏng một task-manager đa CPU thực tế.

---

## Kiến trúc tổng quan

```
┌─────────────────────────────────────────────────────────┐
│                    SMP KERNEL MODULE                    │
│                                                         │
│  CPU0        CPU1        CPU2        CPU3               │
│  ┌──────┐   ┌──────┐   ┌──────┐   ┌──────┐            │
│  │PERCPU│   │PERCPU│   │PERCPU│   │PERCPU│  ← per-cpu  │
│  └──────┘   └──────┘   └──────┘   └──────┘            │
│       ↕           ↕           ↕           ↕            │
│  ┌─────────────────────────────────────────┐           │
│  │         SHARED STATE (protected)        │           │
│  │  ┌─────────┐  ┌──────┐  ┌──────────┐  │           │
│  │  │   RCU   │  │MUTEX │  │SPINLOCK  │  │           │
│  │  │  node   │  │tasks │  │ events   │  │           │
│  │  └─────────┘  └──────┘  └──────────┘  │           │
│  │  ┌─────────────────────────────────┐   │           │
│  │  │       ATOMIC COUNTERS           │   │           │
│  │  └─────────────────────────────────┘   │           │
│  └─────────────────────────────────────────┘           │
└─────────────────────────────────────────────────────────┘
```

---

## 5 Cơ chế đồng bộ hóa

### 1. Per-CPU Variables (`DEFINE_PER_CPU`)
| Đặc điểm | Chi tiết |
|-----------|---------|
| **Nguyên lý** | Mỗi CPU có bản sao biến riêng trong vùng nhớ tách biệt |
| **Không cần lock** | Truy cập từ cùng CPU sau khi disable preemption |
| **API chính** | `get_cpu()` / `put_cpu()`, `__this_cpu_read/write/inc` |
| **Dùng cho** | Counter thống kê, cache riêng CPU, kmem_cache |

```c
DEFINE_PER_CPU(struct cpu_stats, pcpu_stats);

int cpu = get_cpu();                        // disable preemption
__this_cpu_inc(pcpu_stats.task_processed);  // safe, no lock needed
put_cpu();                                  // re-enable preemption
```

---

### 2. RCU – Read-Copy-Update
| Đặc điểm | Chi tiết |
|-----------|---------|
| **Nguyên lý** | Reader không cần lock; writer tạo bản sao mới, swap con trỏ |
| **Ưu điểm** | Read path cực nhanh, zero-overhead trên reader |
| **Giới hạn** | Không dùng trong sleep context bên trong read-side CS |
| **API chính** | `rcu_read_lock/unlock`, `rcu_dereference`, `rcu_assign_pointer`, `call_rcu` |

```c
// Reader (không cần lock, không được sleep)
rcu_read_lock();
node = rcu_dereference(g_rcu_node);
// sử dụng node...
rcu_read_unlock();

// Writer (tạo bản mới, swap, free sau grace-period)
rcu_assign_pointer(g_rcu_node, new_node);
call_rcu(&old_node->rcu, rcu_free_callback);
```

---

### 3. Mutex
| Đặc điểm | Chi tiết |
|-----------|---------|
| **Nguyên lý** | Sleeping lock; thread bị block sẽ ngủ (không waste CPU) |
| **Bắt buộc** | Chỉ dùng trong **process context** (có thể ngủ) |
| **KHÔNG dùng** | Interrupt handler, softirq, atomic context |
| **API chính** | `mutex_lock`, `mutex_unlock`, `mutex_lock_interruptible` |

```c
DEFINE_MUTEX(task_list_mutex);

mutex_lock_interruptible(&task_list_mutex);  // có thể ngủ
// ... truy cập task_list ...
mutex_unlock(&task_list_mutex);
```

---

### 4. Spinlock
| Đặc điểm | Chi tiết |
|-----------|---------|
| **Nguyên lý** | Busy-wait; CPU quay vòng kiểm tra lock liên tục |
| **Dùng được** | Interrupt handler, softirq, atomic context |
| **Yêu cầu** | Giữ lock **cực ngắn** (< vài microseconds) |
| **API chính** | `spin_lock_irqsave`, `spin_unlock_irqrestore` |

```c
DEFINE_SPINLOCK(event_lock);

unsigned long flags;
spin_lock_irqsave(&event_lock, flags);    // disable IRQ + lock
// ... ghi event_ring (thao tác cực ngắn) ...
spin_unlock_irqrestore(&event_lock, flags); // unlock + restore IRQ
```

---

### 5. Atomic Operations
| Đặc điểm | Chi tiết |
|-----------|---------|
| **Nguyên lý** | Thao tác đơn nguyên tử bằng CPU instruction (LOCK XADD...) |
| **Không cần lock** | Phần cứng đảm bảo tính atomic |
| **Giới hạn** | Chỉ dùng cho thao tác đơn giản (inc/dec/add/cmpxchg) |
| **API chính** | `atomic_inc`, `atomic_read`, `atomic_fetch_add`, `atomic64_add` |

```c
static atomic_t g_total_ops;
static atomic64_t g_bytes_total;

atomic_inc(&g_total_ops);              // nguyên tử, thread-safe
atomic64_add(bytes, &g_bytes_total);   // 64-bit atomic
int val = atomic_read(&g_total_ops);   // đọc nguyên tử
```

---

## Khi nào dùng cơ chế nào?

```
Cần bảo vệ dữ liệu?
│
├─ Dữ liệu riêng cho từng CPU?
│   └─► PER-CPU VARIABLE (nhanh nhất, zero contention)
│
├─ Đọc >> Ghi, pointer/struct?
│   └─► RCU (reader zero-overhead)
│
├─ Chỉ 1 biến đơn giản (counter)?
│   └─► ATOMIC (không cần lock)
│
├─ Cần dùng trong interrupt / không được sleep?
│   └─► SPINLOCK + irqsave
│
└─ Ngữ cảnh process, section dài, có thể sleep?
    └─► MUTEX
```

---

## Build & Test

```bash
# Yêu cầu: kernel headers
sudo apt install linux-headers-$(uname -r)   # Ubuntu/Debian
sudo dnf install kernel-devel                 # Fedora/RHEL

# Build
make

# Load module
make load
# hoặc: sudo insmod smp_sync.ko

# Đọc trạng thái
make read
# hoặc: cat /proc/smp_sync

# Xem kernel log
make log
# hoặc: sudo dmesg | grep smp_sync

# Unload
make unload
# hoặc: sudo rmmod smp_sync
```

### Output mẫu `/proc/smp_sync`
```
╔══════════════════════════════════════════════════╗
║   SMP Synchronization Demo — /proc/smp_sync     ║
╚══════════════════════════════════════════════════╝
  Online CPUs: 4  |  Possible CPUs: 4

=== [1] PER-CPU STATISTICS ===
  CPU0: tasks_processed=12       irq_count=0        last_jiffies=4295012
  CPU1: tasks_processed=0        irq_count=0        last_jiffies=0
  ...

=== [2] RCU READ ===
  node: id=10  name='task-node-10'  priority=0

=== [3] MUTEX-PROTECTED TASK LIST ===
  [ 0] tid=0    network packet processing
  [ 3] tid=3    scheduler tick
  ...

=== [4] SPINLOCK-PROTECTED EVENT RING ===
  Total events logged: 15 (ring shows last 15)
  [ 0] CPU0  type=IRQ       jiffies=4295000
  ...

=== [5] ATOMIC COUNTERS ===
  total_ops      = 15
  active_workers = 0
  bytes_total    = 61440
  error_count    = 0
```

---

## Lưu ý an toàn

- **Không dùng mutex trong spinlock section** — gây deadlock
- **Không sleep trong rcu_read_lock** — gây stall grace-period
- **Spinlock phải cực ngắn** — giữ lock lâu làm treo CPU khác
- **Per-cpu cần disable preemption** — dùng `get_cpu()` / `put_cpu()`
- Module được test trên Linux ≥ 5.10 (proc_ops API)