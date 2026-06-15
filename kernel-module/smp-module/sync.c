// SPDX-License-Identifier: GPL-2.0
/**
 * smp_sync.c - Linux Kernel Module minh họa đồng bộ hóa nâng cao trên SMP
 *
 * Tích hợp 5 cơ chế cơ bản VÀ bổ sung 4 thành phần nâng cao:
 * 1. Memory Barriers           — Bảo đảm thứ tự thực thi phần cứng/trình biên dịch
 * 2. Optimized Spinlocks       — Sử dụng rwlock_t và seqlock_t tối ưu hóa đọc/ghi
 * 3. Cache Coherency Practices — Đảm bảo căn chỉnh alignment, chống False Sharing
 * 4. Preemption/IRQ Control    — Điều khiển explicit preemption và interrupt context
 *
 * Build:  make -C /lib/modules/$(uname -r)/build M=$(pwd) modules
 * Load:   sudo insmod smp_sync.ko
 * Test:   sudo cat /proc/smp_sync
 * Unload: sudo rmmod smp_sync
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/percpu.h>
#include <linux/rcupdate.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/smp.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/cpumask.h>
#include <linux/seqlock.h>  /* Thư viện cho seqlock_t */
#include <linux/rwlock.h>   /* Thư viện cho rwlock_t */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SMP Sync Expert Demo");
MODULE_DESCRIPTION("Kernel module minh hoa dong bo hoa SMP nang cao");
MODULE_VERSION("2.0");

/* =========================================================
 * [MỚI BỔ SUNG] CACHE COHERENCY PRACTICES & ALIGNMENT
 * Chống hiện tượng False Sharing bằng cách tách biệt các dữ liệu
 * được ghi độc lập bởi các CPU khác nhau vào các Cacheline riêng biệt.
 * ========================================================= */
struct ____cacheline_aligned_in_smp cpu_network_traffic {
    unsigned long tx_packets;
    unsigned long rx_packets;
};
static DEFINE_PER_CPU(struct cpu_network_traffic, pcpu_traffic);

/* Cấu trúc dữ liệu cấu hình hệ thống: Ít khi thay đổi, đọc rất nhiều */
struct system_runtime_config {
    u32 network_mode;
    u32 max_burst_size;
} ____cacheline_aligned_in_smp;

static struct system_runtime_config global_config;

/* =========================================================
 * [MỚI BỔ SUNG] OPTIMIZED SPINLOCKS (RWLOCK & SEQLOCK)
 * ========================================================= */
/* 1. Read-Write Lock: Nhiều reader song song, nhưng writer độc quyền */
static DEFINE_RWLOCK(packet_queue_rwlock);
#define PACKET_QUEUE_SIZE 8
static int packet_queue[PACKET_QUEUE_SIZE];
static int queue_head = 0;
static int queue_tail = 0;

/* 2. Sequence Lock: Reader không bị block bởi Writer, Reader kiểm tra sequence */
static DEFINE_SEQLOCK(system_time_seqlock);
struct system_precise_time {
    u64 seconds;
    u32 nanoseconds;
};
static struct system_precise_time shared_time_data;

/* =========================================================
 * [MỚI BỔ SUNG] MEMORY BARRIERS (HARDWARE & COMPILER)
 * Minh họa mô hình Lockless Ring Buffer (Single Producer - Single Consumer)
 * ========================================================= */
struct lockless_buffer {
    int data[16];
    unsigned int head; /* Chỉ ghi bởi Producer */
    unsigned int tail; /* Chỉ ghi bởi Consumer */
};
static struct lockless_buffer ring_buf;

static void lockless_producer_enqueue(int val)
{
    unsigned int next_head = (ring_buf.head + 1) % 16;
    
    if (next_head != ring_buf.tail) {
        ring_buf.data[ring_buf.head] = val;
        
        /* * HW MEMORY BARRIER: smp_wmb() đảm bảo dữ liệu trong `data` phải được 
         * ghi vào bộ nhớ trước khi chỉ số `head` mới được cập nhật và public ra các CPU khác.
         */
        smp_wmb();
        
        ring_buf.head = next_head;
    }
}

static int lockless_consumer_dequeue(int *val)
{
    unsigned int current_tail = ring_buf.tail;
    
    if (ring_buf.head == current_tail)
        return -1; /* Rỗng */
        
    *val = ring_buf.data[current_tail];
    
    /* * HW MEMORY BARRIER: smp_rmb() đảm bảo việc đọc `data` hoàn tất 
     * trước khi cập nhật `tail` mới, tránh tình trạng CPU suy đoán (speculative read) dữ liệu cũ.
     */
    smp_rmb();
    
    ring_buf.tail = (current_tail + 1) % 16;
    return 0;
}

/* =========================================================
 * 1. PER-CPU VARIABLES
 * ========================================================= */
struct cpu_stats {
    unsigned long task_processed;
    unsigned long irq_count;
    unsigned long last_jiffies;
};
DEFINE_PER_CPU(struct cpu_stats, pcpu_stats);

static void percpu_update_stats(void)
{
    int cpu = get_cpu(); /* Disable preemption */

    __this_cpu_inc(pcpu_stats.task_processed);
    __this_cpu_write(pcpu_stats.last_jiffies, jiffies);
    
    /* Cập nhật phần cacheline-aligned dữ liệu mạng */
    __this_cpu_inc(pcpu_traffic.rx_packets);

    put_cpu(); /* Re-enable preemption */
}

/* =========================================================
 * 2. RCU (Read-Copy-Update)
 * ========================================================= */
struct rcu_node {
    int   id;
    char  name[32];
    int   priority;
    struct rcu_head rcu;
};
static struct rcu_node __rcu *g_rcu_node;
static DEFINE_SPINLOCK(rcu_writer_lock);

static void rcu_free_callback(struct rcu_head *rh)
{
    struct rcu_node *old = container_of(rh, struct rcu_node, rcu);
    kfree(old);
}

static int rcu_update_node(int new_id, const char *new_name, int new_prio)
{
    struct rcu_node *new_node, *old_node;

    new_node = kmalloc(sizeof(*new_node), GFP_KERNEL);
    if (!new_node)
        return -ENOMEM;

    new_node->id       = new_id;
    new_node->priority = new_prio;
    strncpy(new_node->name, new_name, sizeof(new_node->name) - 1);
    new_node->name[sizeof(new_node->name) - 1] = '\0';

    spin_lock(&rcu_writer_lock);
    old_node = rcu_dereference_protected(g_rcu_node, lockdep_is_held(&rcu_writer_lock));
    rcu_assign_pointer(g_rcu_node, new_node);
    spin_unlock(&rcu_writer_lock);

    if (old_node)
        call_rcu(&old_node->rcu, rcu_free_callback);

    return 0;
}

/* =========================================================
 * 3. MUTEX
 * ========================================================= */
static DEFINE_MUTEX(task_list_mutex);
#define MAX_TASKS 16
struct task_entry {
    int   tid;
    char  desc[64];
    ktime_t created_at;
};
static struct task_entry task_list[MAX_TASKS];
static int task_count;

static int mutex_add_task(int tid, const char *desc)
{
    int ret = 0;
    if (mutex_lock_interruptible(&task_list_mutex))
        return -EINTR;

    if (task_count >= MAX_TASKS) {
        ret = -ENOSPC;
        goto out;
    }

    task_list[task_count].tid        = tid;
    task_list[task_count].created_at = ktime_get();
    strncpy(task_list[task_count].desc, desc, sizeof(task_list[task_count].desc) - 1);
    task_list[task_count].desc[sizeof(task_list[task_count].desc)-1] = '\0';
    task_count++;

out:
    mutex_unlock(&task_list_mutex);
    return ret;
}

/* =========================================================
 * 4. SPINLOCK & [MỚI BỔ SUNG] EXPLICIT PREEMPTION/IRQ CONTROL
 * Trực tiếp can thiệp và kiểm soát ngữ cảnh chạy thực tế
 * ========================================================= */
static DEFINE_SPINLOCK(event_lock);
#define MAX_EVENTS 32
struct event_entry {
    int           cpu;
    unsigned long jiffies_stamp;
    unsigned int  event_type;
};
static struct event_entry event_ring[MAX_EVENTS];
static int event_head;
static int event_total;

static void spinlock_log_event(unsigned int type)
{
    unsigned long flags;

    /* * EXPLICIT IRQ CONTROL: Vừa khóa Spinlock vừa vô hiệu hóa ngắt cục bộ của CPU này.
     * Tránh hoàn toàn lỗi Deadlock nếu một Interrupt Handler nhảy vào chiếm quyền CPU 
     * và cố gắng lấy cùng một spinlock này.
     */
    spin_lock_irqsave(&event_lock, flags);

    event_ring[event_head].cpu          = smp_processor_id();
    event_ring[event_head].jiffies_stamp = jiffies;
    event_ring[event_head].event_type   = type;
    event_head = (event_head + 1) % MAX_EVENTS;
    event_total++;

    spin_unlock_irqrestore(&event_lock, flags);
}

/* Minh họa Explicit Preemption Control độc lập */
static void critical_hardware_io_simulation(void)
{
    /* * EXPLICIT PREEMPTION CONTROL:
     * Tắt tính năng hoán vị tiến trình (preemption). Đoạn mã bên dưới cam đoan 
     * chạy liên tục không bị luồng khác chiếm CPU cho tới khi gọi preempt_enable().
     */
    preempt_disable();
    
    /* Thao tác thanh ghi phần cứng giả định... */
    barrier(); /* Compiler Barrier: Không cho phép GCC sắp xếp lại lệnh xung quanh điểm này */
    
    preempt_enable();
}

/* =========================================================
 * 5. ATOMIC OPERATIONS
 * ========================================================= */
static atomic_t g_total_ops;
static atomic_t g_active_workers;
static atomic64_t g_bytes_total;
static atomic_t g_error_count;

static void atomic_simulate_work(int worker_id, long bytes)
{
    atomic_inc(&g_active_workers);
    atomic_inc(&g_total_ops);
    atomic64_add(bytes, &g_bytes_total);
    if (bytes < 0)
        atomic_inc(&g_error_count);
    atomic_dec(&g_active_workers);
}

/* =========================================================
 * HELPER FUNCTIONS CHO OPTIMIZED LOCKS (MÔ PHỎNG TẢI)
 * ========================================================= */
static void rwlock_write_packet(int pkt_id)
{
    write_lock(&packet_queue_rwlock);
    if ((queue_head + 1) % PACKET_QUEUE_SIZE != queue_tail) {
        packet_queue[queue_head] = pkt_id;
        queue_head = (queue_head + 1) % PACKET_QUEUE_SIZE;
    }
    write_unlock(&packet_queue_rwlock);
}

static void seqlock_update_time(void)
{
    write_seqlock(&system_time_seqlock);
    shared_time_data.seconds++;
    shared_time_data.nanoseconds = (shared_time_data.nanoseconds + 1000) % 1000000000;
    write_sequnlock(&system_time_seqlock);
}

/* =========================================================
 * PROCFS INTERFACE (/proc/smp_sync)
 * ========================================================= */
static int smp_sync_show(struct seq_file *m, void *v)
{
    int cpu, i;
    struct rcu_node *node;
    unsigned long flags;
    unsigned int seq;
    struct system_precise_time local_time;

    seq_puts(m,
        "╔══════════════════════════════════════════════════╗\n"
        "║   SMP Synchronization Expert Demo — v2.0         ║\n"
        "╚══════════════════════════════════════════════════╝\n");
    seq_printf(m, "  Online CPUs: %d  |  Possible CPUs: %d\n",
               num_online_cpus(), num_possible_cpus());

    /* 1. PER-CPU & CACHEALIGN */
    seq_puts(m, "\n=== [1] PER-CPU STATISTICS & CACHE ALIGNMENT ===\n");
    for_each_possible_cpu(cpu) {
        struct cpu_stats *st = per_cpu_ptr(&pcpu_stats, cpu);
        struct cpu_network_traffic *tf = per_cpu_ptr(&pcpu_traffic, cpu);
        seq_printf(m, "  CPU%d: processed=%-6lu | cacheline-aligned rx_packets=%-6lu\n",
                   cpu, st->task_processed, tf->rx_packets);
    }

    /* 2. RCU */
    seq_puts(m, "\n=== [2] RCU READ ===\n");
    rcu_read_lock();
    node = rcu_dereference(g_rcu_node);
    if (node)
        seq_printf(m, "  node: id=%d  name='%s'  priority=%d\n", node->id, node->name, node->priority);
    rcu_read_unlock();

    /* 3. MUTEX */
    seq_puts(m, "\n=== [3] MUTEX-PROTECTED TASK LIST ===\n");
    mutex_lock(&task_list_mutex);
    if (task_count == 0) {
        seq_puts(m, "  (empty)\n");
    } else {
        for (i = 0; i < task_count; i++) {
            seq_printf(m, "  [%2d] tid=%-4d  %s\n", i, task_list[i].tid, task_list[i].desc);
        }
    }
    mutex_unlock(&task_list_mutex);

    /* 4. SPINLOCK & IRQ CONTROL */
    seq_puts(m, "\n=== [4] SPINLOCK-PROTECTED EVENT RING ===\n");
    spin_lock_irqsave(&event_lock, flags);
    seq_printf(m, "  Total events logged: %d (ring capacity: %d)\n", event_total, MAX_EVENTS);
    spin_unlock_irqrestore(&event_lock, flags);

    /* 5. ATOMIC COUNTERS */
    seq_puts(m, "\n=== [5] ATOMIC COUNTERS ===\n");
    seq_printf(m, "  total_ops = %-6d | active_workers = %-6d | bytes_total = %lld\n",
               atomic_read(&g_total_ops), atomic_read(&g_active_workers), (long long)atomic64_read(&g_bytes_total));

    /* 6. OPTIMIZED SPINLOCKS (RWLOCK & SEQLOCK) */
    seq_puts(m, "\n=== [6] OPTIMIZED SPINLOCKS (RWLOCK & SEQLOCK) ===\n");
    
    // Đọc song song từ RWLOCK
    read_lock(&packet_queue_rwlock);
    seq_printf(m, "  [RWLock Reading] Queue pointers -> Head: %d, Tail: %d\n", queue_head, queue_tail);
    read_unlock(&packet_queue_rwlock);

    // Đọc không block từ SEQLOCK (Lockless Read Reader Loop)
    do {
        seq = read_seqbegin(&system_time_seqlock);
        local_time = shared_time_data;
    } while (read_seqretry(&system_time_seqlock, seq));
    
    seq_printf(m, "  [SeqLock Reading] Precise Shared Time: %llu s, %u ns\n", 
               local_time.seconds, local_time.nanoseconds);

    /* 7. MEMORY BARRIERS LOCKLESS BUFFER STATUS */
    seq_puts(m, "\n=== [7] MEMORY BARRIER LOCKLESS BUFFER ===\n");
    seq_printf(m, "  Lockless Ring Buffer indices -> Head: %u, Tail: %u\n", ring_buf.head, ring_buf.tail);

    seq_puts(m, "\n");
    return 0;
}

static int smp_sync_open(struct inode *inode, struct file *file)
{
    return single_open(file, smp_sync_show, NULL);
}

static const struct proc_ops smp_sync_fops = {
    .proc_open    = smp_sync_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/* =========================================================
 * KTHREAD WORKER — MÔ PHỎNG TẢI SMP THỰC TẾ
 * ========================================================= */
static struct task_struct *demo_thread;

static int demo_worker_fn(void *data)
{
    int iter = 0;
    int dummy_val;
    static const char * const task_descs[] = {
        "network packet processing", "disk I/O completion", "timer callback"
    };

    pr_info("smp_sync: demo_worker started on CPU%d\n", smp_processor_id());

    while (!kthread_should_stop()) {
        /* Core [1] Per-CPU variables update */
        percpu_update_stats();

        /* Core [2] RCU update */
        if (iter % 5 == 0) {
            char name[32];
            snprintf(name, sizeof(name), "task-node-%d", iter);
            rcu_update_node(iter, name, iter % 10);
        }

        /* Core [3] Mutex (process context context can sleep) */
        if (iter % 3 == 0 && task_count < MAX_TASKS) {
            mutex_add_task(iter, task_descs[iter % ARRAY_SIZE(task_descs)]);
        }

        /* Core [4] Spinlock & Explicit IRQ Control */
        spinlock_log_event(iter % 3);

        /* Core [5] Atomic operations */
        atomic_simulate_work(iter, (long)(iter * 512));

        /* NÂNG CAO [1]: Memory Barriers - Enqueue / Dequeue lockless */
        lockless_producer_enqueue(iter);
        if (iter % 2 == 0) {
            lockless_consumer_dequeue(&dummy_val);
        }

        /* NÂNG CAO [2]: Optimized Locks */
        rwlock_write_packet(iter);
        seqlock_update_time();

        /* NÂNG CAO [4]: Explicit Preemption Control */
        critical_hardware_io_simulation();

        msleep(200); /* Chờ 200ms, cho phép scheduler hoán vị tự nhiên */
        iter++;
    }

    pr_info("smp_sync: demo_worker stopped\n");
    return 0;
}

/* =========================================================
 * MODULE INIT / EXIT
 * ========================================================= */
static struct proc_dir_entry *proc_entry;

static int __init smp_sync_init(void)
{
    int cpu, ret;

    pr_info("smp_sync: === Module Loading on %d-CPU System ===\n", num_possible_cpus());

    /* Khởi tạo Cache-Coherency Data */
    global_config.network_mode = 1;
    global_config.max_burst_size = 64;

    /* Khởi tạo Per-CPU */
    for_each_possible_cpu(cpu) {
        struct cpu_stats *st = per_cpu_ptr(&pcpu_stats, cpu);
        struct cpu_network_traffic *tf = per_cpu_ptr(&pcpu_traffic, cpu);
        st->task_processed = 0;
        st->irq_count      = 0;
        st->last_jiffies   = 0;
        tf->tx_packets     = 0;
        tf->rx_packets     = 0;
    }

    /* Khởi tạo RCU Node */
    ret = rcu_update_node(0, "initial-node", 5);
    if (ret) return ret;

    /* Khởi tạo Lockless Ring Buffer Struct indices */
    ring_buf.head = 0;
    ring_buf.tail = 0;

    /* Khởi tạo Precise shared time cho Seqlock */
    shared_time_data.seconds = 0;
    shared_time_data.nanoseconds = 0;

    /* Tạo /proc/smp_sync entry */
    proc_entry = proc_create("smp_sync", 0444, NULL, &smp_sync_fops);
    if (!proc_entry) return -ENOMEM;

    /* Khởi chạy demo thread */
    demo_thread = kthread_run(demo_worker_fn, NULL, "smp_sync_worker");
    if (IS_ERR(demo_thread)) {
        ret = PTR_ERR(demo_thread);
        proc_remove(proc_entry);
        return ret;
    }

    return 0;
}

static void __exit smp_sync_exit(void)
{
    struct rcu_node *node;

    if (demo_thread)
        kthread_stop(demo_thread);

    proc_remove(proc_entry);

    spin_lock(&rcu_writer_lock);
    node = rcu_dereference_protected(g_rcu_node, lockdep_is_held(&rcu_writer_lock));
    rcu_assign_pointer(g_rcu_node, NULL);
    spin_unlock(&rcu_writer_lock);

    if (node) {
        synchronize_rcu();
        kfree(node);
    }

    pr_info("smp_sync: Module unloaded cleanly.\n");
}

module_init(smp_sync_init);
module_exit(smp_sync_exit);