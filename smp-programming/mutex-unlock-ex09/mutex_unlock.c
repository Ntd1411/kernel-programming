/*
 * Mutex Unlock Example
 * 
 * Demonstrates both fast and slow paths of mutex_unlock().
 * Fast path: Atomic clear when no waiters
 * Slow path: Wake up first waiter when wait queue is not empty
 * 
 * Based on kernel implementation from kernel/locking/mutex.c
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>

/* Waiter flags - stored in low bits of owner field */
#define MUTEX_FLAG_WAITERS  0x01

/* Simplified waiter structure */
struct mutex_waiter {
    pthread_t task;
    pthread_cond_t cond;
    int woken;
    struct mutex_waiter *next;
};

/* Mutex structure */
typedef struct {
    atomic_ulong owner;  /* Owner thread | flags */
    pthread_mutex_t wait_lock;
    struct mutex_waiter *wait_list;
} mutex_t;

/* Statistics */
static atomic_int fast_unlock_count = 0;
static atomic_int slow_unlock_count = 0;

/* Shared counter */
static int shared_counter = 0;
static mutex_t my_mutex = { 0, PTHREAD_MUTEX_INITIALIZER, NULL };

/*
 * __mutex_unlock_fast - Try fast path unlock
 * 
 * Returns true if successful (no waiters), false otherwise.
 * Uses atomic compare-exchange to check and clear owner atomically.
 */
static inline int __mutex_unlock_fast(mutex_t *lock)
{
    unsigned long curr = (unsigned long)pthread_self();
    unsigned long expected = curr;
    
    /* 
     * Try to atomically change owner from curr to 0
     * This succeeds only if there are no WAITERS flag bits set
     */
    if (atomic_compare_exchange_strong_explicit(
            &lock->owner,
            &expected,
            0UL,
            memory_order_release,
            memory_order_relaxed)) {
        return 1;  /* Fast path success */
    }
    
    return 0;  /* Slow path needed */
}

/*
 * __mutex_unlock_slowpath - Wake up first waiter
 * 
 * Called when there are waiters. Picks first waiter from
 * wait_list and wakes it up with direct ownership handoff.
 */
static void __mutex_unlock_slowpath(mutex_t *lock)
{
    pthread_mutex_lock(&lock->wait_lock);
    
    if (lock->wait_list != NULL) {
        /* Get first waiter and remove from list */
        struct mutex_waiter *waiter = lock->wait_list;
        lock->wait_list = waiter->next;
        
        /* Direct handoff: transfer ownership to waiter */
        unsigned long new_owner = waiter->task;
        
        /* Preserve WAITERS flag if there are still waiters */
        if (lock->wait_list != NULL) {
            new_owner |= MUTEX_FLAG_WAITERS;
        }
        
        atomic_store_explicit(&lock->owner, new_owner, memory_order_release);
        
        /* Wake it up */
        waiter->woken = 1;
        pthread_cond_signal(&waiter->cond);
    } else {
        /* No waiters, just clear owner */
        atomic_store_explicit(&lock->owner, 0, memory_order_release);
    }
    
    pthread_mutex_unlock(&lock->wait_lock);
}

/*
 * mutex_unlock - Release the mutex
 * 
 * Fast path: If no waiters, just atomic clear
 * Slow path: Wake up first waiter
 */
void mutex_unlock(mutex_t *lock)
{
    if (__mutex_unlock_fast(lock)) {
        atomic_fetch_add(&fast_unlock_count, 1);
        return;
    }
    
    /* Slow path */
    atomic_fetch_add(&slow_unlock_count, 1);
    __mutex_unlock_slowpath(lock);
}

/*
 * mutex_lock - Acquire the mutex (simplified)
 */
void mutex_lock(mutex_t *lock)
{
    unsigned long curr = (unsigned long)pthread_self();
    unsigned long expected = 0;
    
    /* Try fast path */
    if (atomic_compare_exchange_strong_explicit(
            &lock->owner,
            &expected,
            curr,
            memory_order_acquire,
            memory_order_relaxed)) {
        return;
    }
    
    /* Slow path - add to wait list and sleep */
    struct mutex_waiter waiter;
    waiter.task = curr;
    waiter.woken = 0;
    waiter.next = NULL;
    pthread_cond_init(&waiter.cond, NULL);
    
    pthread_mutex_lock(&lock->wait_lock);
    
    /* Add to wait list */
    if (lock->wait_list == NULL) {
        lock->wait_list = &waiter;
    } else {
        struct mutex_waiter *w = lock->wait_list;
        while (w->next != NULL) w = w->next;
        w->next = &waiter;
    }
    
    /* Set WAITERS flag */
    unsigned long owner = atomic_load(&lock->owner);
    atomic_store(&lock->owner, owner | MUTEX_FLAG_WAITERS);
    
    /* Wait until woken */
    while (!waiter.woken) {
        pthread_cond_wait(&waiter.cond, &lock->wait_lock);
    }
    
    /* Acquired! (unlock did direct handoff - we already own it) */
    
    pthread_mutex_unlock(&lock->wait_lock);
    pthread_cond_destroy(&waiter.cond);
}

/*
 * Critical section
 */
void critical_section(int thread_id, int iteration)
{
    mutex_lock(&my_mutex);
    
    printf("[Thread %d:%d] Counter: %d -> ", thread_id, iteration, shared_counter);
    fflush(stdout);
    
    int old_value = shared_counter;
    usleep(2000);  /* Simulate work */
    shared_counter = old_value + 1;
    
    printf("%d\n", shared_counter);
    
    mutex_unlock(&my_mutex);
}

void *thread_func(void *arg)
{
    int thread_id = *(int *)arg;
    
    for (int i = 0; i < 3; i++) {
        critical_section(thread_id, i + 1);
        usleep(100);
    }
    
    return NULL;
}

int main(void)
{
    pthread_t threads[4];
    int thread_ids[4];
    
    printf("=== Mutex Unlock Fast & Slow Path Demo ===\n");
    printf("Demonstrating unlock optimization with waiter detection\n\n");
    
    printf("Fast path:\n");
    printf("  if (owner == current && no WAITERS flag)\n");
    printf("      owner = 0;  // Atomic clear\n");
    printf("      return;     // Done\n\n");
    
    printf("Slow path:\n");
    printf("  Pick first waiter from wait_list\n");
    printf("  Wake it up (wake_q_add + wake_up_q)\n");
    printf("  Clear owner\n\n");
    
    printf("Note: task_struct is cache-aligned, so low 7 bits\n");
    printf("      of owner field can be used for flags.\n\n");
    
    /* Create 4 threads with contention */
    printf("Creating 4 threads...\n\n");
    
    for (int i = 0; i < 4; i++) {
        thread_ids[i] = i + 1;
        pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]);
    }
    
    /* Wait for completion */
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }
    
    /* Display results */
    printf("\n=== Results ===\n");
    printf("Final counter: %d (expected: 12)\n", shared_counter);
    printf("Fast path unlocks: %d\n", atomic_load(&fast_unlock_count));
    printf("Slow path unlocks: %d\n", atomic_load(&slow_unlock_count));
    
    int total = atomic_load(&fast_unlock_count) + atomic_load(&slow_unlock_count);
    if (total > 0) {
        float fast_rate = 100.0f * atomic_load(&fast_unlock_count) / total;
        printf("Fast path rate: %.1f%%\n", fast_rate);
    }
    
    if (shared_counter == 12) {
        printf("\n✓ SUCCESS: Mutex unlock correctly handled waiters\n");
        return 0;
    } else {
        printf("\n!!  ERROR: Counter mismatch\n");
        return 1;
    }
}
