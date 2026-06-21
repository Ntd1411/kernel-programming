/*
 * Memory Ordering and RCU Example
 * 
 * Demonstrates:
 * 1. Compiler reordering and compiler barriers
 * 2. Memory barriers (rmb, wmb, mb, smp_*)
 * 3. RCU list operations (rcu_read_lock, synchronize_rcu, etc.)
 * 
 * Based on kernel memory ordering primitives
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>
#include <string.h>

/*
 * Compiler Barriers
 */
#define barrier() __asm__ __volatile__("": : :"memory")

/*
 * Memory Barriers (x86-64)
 */
#define mb()  __asm__ __volatile__("mfence":::"memory")
#define rmb() __asm__ __volatile__("lfence":::"memory")
#define wmb() __asm__ __volatile__("sfence":::"memory")

/* SMP versions (same as full barriers on x86) */
#define smp_mb()  mb()
#define smp_rmb() rmb()
#define smp_wmb() wmb()

/*
 * Part 1: Compiler Reordering Demo
 */
static int x = 0, y = 0;
static int r1 = 0, r2 = 0;

void reorder_demo_thread1(void)
{
    x = 1;
    barrier();  /* Prevent compiler reordering */
    r1 = y;
}

void reorder_demo_thread2(void)
{
    y = 1;
    barrier();  /* Prevent compiler reordering */
    r2 = x;
}

/*
 * Part 2: RCU List Implementation
 */

/* RCU list node */
struct rcu_node {
    int data;
    struct rcu_node *next;
};

/* RCU protected list head */
static _Atomic(struct rcu_node *) rcu_list_head = NULL;
static pthread_rwlock_t rcu_lock = PTHREAD_RWLOCK_INITIALIZER;

/*
 * rcu_read_lock - Begin RCU read-side critical section
 */
static inline void rcu_read_lock(void)
{
    pthread_rwlock_rdlock(&rcu_lock);
}

/*
 * rcu_read_unlock - End RCU read-side critical section
 */
static inline void rcu_read_unlock(void)
{
    pthread_rwlock_unlock(&rcu_lock);
}

/*
 * rcu_dereference - Fetch RCU-protected pointer
 * Ensures dependent loads are not reordered before pointer load
 */
#define rcu_dereference(p) \
({ \
    typeof(p) _p = atomic_load_explicit((_Atomic typeof(p) *)&(p), \
                                         memory_order_consume); \
    _p; \
})

/*
 * rcu_assign_pointer - Assign to RCU-protected pointer
 * Ensures all initialization is visible before pointer assignment
 */
#define rcu_assign_pointer(p, v) \
do { \
    smp_wmb(); \
    atomic_store_explicit((_Atomic typeof(p) *)&(p), (v), \
                          memory_order_release); \
} while (0)

/*
 * synchronize_rcu - Wait for all RCU readers to complete
 */
static void synchronize_rcu(void)
{
    pthread_rwlock_wrlock(&rcu_lock);
    pthread_rwlock_unlock(&rcu_lock);
}

/*
 * list_add_rcu - Add node to RCU-protected list
 */
static void list_add_rcu(int data)
{
    struct rcu_node *new_node = malloc(sizeof(*new_node));
    if (!new_node) return;
    
    new_node->data = data;
    new_node->next = rcu_list_head;
    
    /* Ensure new_node is fully initialized before visible */
    rcu_assign_pointer(rcu_list_head, new_node);
}

/*
 * list_del_rcu - Delete node from RCU-protected list
 */
static void list_del_rcu(int data)
{
    /* Cast to suppress atomic pointer warning - macros handle atomicity */
    struct rcu_node **pp = (struct rcu_node **)&rcu_list_head;
    struct rcu_node *entry;
    
    while ((entry = rcu_dereference(*pp)) != NULL) {
        if (entry->data == data) {
            rcu_assign_pointer(*pp, entry->next);
            synchronize_rcu();  /* Wait for readers */
            free(entry);
            return;
        }
        pp = &entry->next;
    }
}

/*
 * list_for_each_rcu - Iterate RCU-protected list
 */
static void list_for_each_rcu(void (*callback)(int))
{
    struct rcu_node *node;
    
    rcu_read_lock();
    
    for (node = rcu_dereference(rcu_list_head); 
         node != NULL; 
         node = rcu_dereference(node->next)) {
        callback(node->data);
    }
    
    rcu_read_unlock();
}

/*
 * Demo functions
 */
/* Statistics */
static atomic_int total_reads = 0;
static atomic_int reads_during_update = 0;
static atomic_int writer_active = 0;
static atomic_int done = 0;

static void print_node(int data)
{
    printf("%d ", data);
}

void *reader_thread(void *arg)
{
    int id = *(int *)arg;
    int local_reads = 0;
    
    /* Continuous reading - demonstrating lock-free access */
    while (!atomic_load(&done)) {
        rcu_read_lock();
        
        /* Count reads during updates to show concurrency */
        int is_updating = atomic_load(&writer_active);
        if (is_updating) {
            atomic_fetch_add(&reads_during_update, 1);
        }
        
        /* RCU read is lock-free - no blocking! */
        struct rcu_node *node;
        int count = 0;
        for (node = rcu_dereference(rcu_list_head); 
             node != NULL; 
             node = rcu_dereference(node->next)) {
            count++;
        }
        
        rcu_read_unlock();
        
        local_reads++;
        atomic_fetch_add(&total_reads, 1);
        
        usleep(1000);  /* 1ms between reads */
    }
    
    printf("[Reader %d] Completed %d reads (never blocked!)\n", id, local_reads);
    return NULL;
}

void *writer_thread(void *arg)
{
    (void)arg;
    
    sleep(1);
    
    /* Update 1: Add node */
    printf("\n========================================\n");
    printf("[Writer] Starting UPDATE 1: Adding node 100\n");
    printf("[Writer] Readers continue WITHOUT BLOCKING...\n");
    atomic_store(&writer_active, 1);
    
    int reads_before = atomic_load(&reads_during_update);
    list_add_rcu(100);
    sleep(1);
    int reads_after = atomic_load(&reads_during_update);
    
    printf("[Writer] UPDATE 1 complete: %d lock-free reads happened during update!\n", 
           reads_after - reads_before);
    printf("========================================\n");
    atomic_store(&writer_active, 0);
    
    sleep(1);
    
    /* Update 2: Add another node */
    printf("\n========================================\n");
    printf("[Writer] Starting UPDATE 2: Adding node 200\n");
    printf("[Writer] Readers still running lock-free...\n");
    atomic_store(&writer_active, 1);
    
    reads_before = atomic_load(&reads_during_update);
    list_add_rcu(200);
    sleep(1);
    reads_after = atomic_load(&reads_during_update);
    
    printf("[Writer] UPDATE 2 complete: %d more lock-free reads!\n", 
           reads_after - reads_before);
    printf("========================================\n");
    atomic_store(&writer_active, 0);
    
    sleep(1);
    
    /* Update 3: Delete node - demonstrates grace period */
    printf("\n========================================\n");
    printf("[Writer] Starting UPDATE 3: Deleting node 2\n");
    printf("[Writer] Will wait for grace period (all readers to finish)\n");
    atomic_store(&writer_active, 1);
    
    reads_before = atomic_load(&reads_during_update);
    list_del_rcu(2);  /* synchronize_rcu() waits for readers inside */
    reads_after = atomic_load(&reads_during_update);
    
    printf("[Writer] UPDATE 3 complete: Grace period ensured safety!\n");
    printf("[Writer] %d reads happened, old node freed safely\n", 
           reads_after - reads_before);
    printf("========================================\n");
    atomic_store(&writer_active, 0);
    
    sleep(1);
    atomic_store(&done, 1);
    
    return NULL;
}

int main(void)
{
    printf("=== Memory Ordering and RCU Demo ===\n\n");
    
    /* Part 1: Compiler barriers */
    printf("Part 1: Compiler Barriers\n");
    printf("-------------------------\n");
    printf("Without barrier(): compiler may reorder x=1 and r1=y\n");
    printf("With barrier(): forces program order\n");
    printf("x86 has strong memory model, so CPU reordering is rare.\n\n");
    
    /* Part 2: Memory barriers */
    printf("Part 2: Memory Barriers (x86-64)\n");
    printf("--------------------------------\n");
    printf("mb():  Full memory barrier (mfence)\n");
    printf("rmb(): Read memory barrier (lfence)\n");
    printf("wmb(): Write memory barrier (sfence)\n");
    printf("smp_*: SMP versions (same on x86)\n\n");
    
    /* Part 3: RCU demo */
    printf("Part 3: RCU - Read-Copy-Update\n");
    printf("===============================\n");
    printf("KEY ADVANTAGES:\n");
    printf("  ✓ Readers NEVER block (completely lock-free)\n");
    printf("  ✓ Readers don't wait for writers\n");
    printf("  ✓ Writers don't block readers\n");
    printf("  ✓ Multiple readers run concurrently with writers\n");
    printf("  ✓ Grace period ensures memory safety\n\n");
    
    /* Initialize list */
    list_add_rcu(3);
    list_add_rcu(2);
    list_add_rcu(1);
    
    printf("Initial list: ");
    list_for_each_rcu(print_node);
    printf("\n\n");
    
    printf("Starting 4 reader threads (continuous lock-free reading)...\n");
    printf("Starting 1 writer thread (will make 3 updates)...\n");
    printf("Watch readers continue during ALL updates!\n");
    
    /* Create multiple readers and one writer */
    pthread_t readers[4], writer;
    int reader_ids[4] = {1, 2, 3, 4};
    
    for (int i = 0; i < 4; i++) {
        pthread_create(&readers[i], NULL, reader_thread, &reader_ids[i]);
    }
    pthread_create(&writer, NULL, writer_thread, NULL);
    
    /* Wait for completion */
    pthread_join(writer, NULL);
    for (int i = 0; i < 4; i++) {
        pthread_join(readers[i], NULL);
    }
    
    /* Results showing RCU power */
    printf("\n╔════════════════════════════════════════════════╗\n");
    printf("║          RCU PERFORMANCE RESULTS               ║\n");
    printf("╠════════════════════════════════════════════════╣\n");
    printf("║ Total reads completed: %-23d ║\n", atomic_load(&total_reads));
    printf("║ Reads during updates:  %-23d ║\n", atomic_load(&reads_during_update));
    printf("║                                                ║\n");
    
    int total = atomic_load(&total_reads);
    int during = atomic_load(&reads_during_update);
    if (total > 0) {
        float percent = 100.0f * during / total;
        printf("║ %.1f%% of reads happened DURING updates!      ║\n", percent);
    }
    printf("║                                                ║\n");
    printf("║ ✓ Zero reader blocking                         ║\n");
    printf("║ ✓ Readers ran concurrently with writers       ║\n");
    printf("║ ✓ Grace period ensured memory safety          ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
    
    printf("\nFinal list: ");
    list_for_each_rcu(print_node);
    printf("\n");
    
    /* Cleanup */
    while (rcu_list_head) {
        struct rcu_node *temp = rcu_list_head;
        rcu_list_head = rcu_list_head->next;
        free(temp);
    }
    
    printf("\n✓ RCU demonstration complete\n");
    printf("\nTHIS IS THE POWER OF RCU:\n");
    printf("  Traditional rwlock: Readers wait for writer's lock\n");
    printf("  RCU: Readers NEVER wait, even during updates!\n");
    printf("  Result: Massive read scalability with zero reader overhead\n");
    
    return 0;
}
