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
    struct rcu_node **pp = &rcu_list_head;
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
static void print_node(int data)
{
    printf("  Node: %d\n", data);
}

static atomic_int reader_count = 0;

void *reader_thread(void *arg)
{
    int id = *(int *)arg;
    
    for (int i = 0; i < 5; i++) {
        atomic_fetch_add(&reader_count, 1);
        
        printf("[Reader %d] Reading list:\n", id);
        list_for_each_rcu(print_node);
        
        usleep(100000);  /* 100ms */
    }
    
    return NULL;
}

void *writer_thread(void *arg)
{
    (void)arg;
    
    sleep(1);
    printf("\n[Writer] Adding node 100\n");
    list_add_rcu(100);
    
    sleep(1);
    printf("\n[Writer] Adding node 200\n");
    list_add_rcu(200);
    
    sleep(1);
    printf("\n[Writer] Deleting node 2\n");
    list_del_rcu(2);
    
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
    printf("Part 3: RCU List Operations\n");
    printf("---------------------------\n");
    printf("RCU allows lock-free reads while updates happen\n\n");
    
    /* Initialize list */
    list_add_rcu(3);
    list_add_rcu(2);
    list_add_rcu(1);
    
    printf("Initial list: 1 -> 2 -> 3\n\n");
    
    /* Create reader and writer threads */
    pthread_t readers[2], writer;
    int reader_ids[2] = {1, 2};
    
    for (int i = 0; i < 2; i++) {
        pthread_create(&readers[i], NULL, reader_thread, &reader_ids[i]);
    }
    pthread_create(&writer, NULL, writer_thread, NULL);
    
    /* Wait for completion */
    for (int i = 0; i < 2; i++) {
        pthread_join(readers[i], NULL);
    }
    pthread_join(writer, NULL);
    
    /* Results */
    printf("\n=== Results ===\n");
    printf("Total reads: %d\n", atomic_load(&reader_count));
    printf("\nFinal list:\n");
    list_for_each_rcu(print_node);
    
    /* Cleanup */
    while (rcu_list_head) {
        struct rcu_node *temp = rcu_list_head;
        rcu_list_head = rcu_list_head->next;
        free(temp);
    }
    
    printf("\n✓ RCU operations completed successfully\n");
    return 0;
}
