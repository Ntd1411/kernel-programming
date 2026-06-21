/*
 * Mutex Lock Fast Path Example
 * 
 * Demonstrates the fast path of mutex_lock() using atomic compare-and-exchange.
 * This is the optimistic case where the mutex is not contended.
 * 
 * Based on kernel implementation from kernel/locking/mutex.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>

/* Simplified mutex structure */
typedef struct {
    atomic_ulong owner;  /* Current owner (task pointer) or 0 if unlocked */
} mutex_t;

/* Statistics */
static atomic_int fast_path_success = 0;
static atomic_int fast_path_failure = 0;

/* Shared counter protected by mutex */
static int shared_counter = 0;
static mutex_t my_mutex = { 0 };

/*
 * might_sleep - Debug check that we're in a sleepable context
 * In real kernel, this would trigger warnings if called from atomic context
 */
static inline void might_sleep(void)
{
    /* In user-space, we just note that we could sleep */
}

/*
 * __mutex_trylock_fast - Try to acquire mutex via fast path
 * 
 * Returns true if successfully acquired, false otherwise.
 * 
 * Uses atomic_compare_exchange to atomically:
 * 1. Check if owner is 0 (unlocked)
 * 2. If yes, set owner to current thread
 * 3. Return success/failure
 */
static inline int __mutex_trylock_fast(mutex_t *lock)
{
    unsigned long curr = (unsigned long)pthread_self();
    unsigned long expected = 0;
    
    /* Atomic compare-and-exchange with acquire semantics */
    if (atomic_compare_exchange_strong_explicit(
            &lock->owner, 
            &expected,    /* Expected: 0 (unlocked) */
            curr,         /* New value: current thread */
            memory_order_acquire,
            memory_order_relaxed)) {
        return 1;  /* Success - fast path */
    }
    
    return 0;  /* Failed - mutex already held */
}

/*
 * mutex_lock - Acquire the mutex
 * 
 * Fast path: Try atomic acquisition
 * Slow path: (Not implemented in this example - see ex08)
 */
void mutex_lock(mutex_t *lock)
{
    might_sleep();
    
    if (__mutex_trylock_fast(lock)) {
        /* Fast path success! */
        atomic_fetch_add(&fast_path_success, 1);
        return;
    }
    
    /* 
     * Fast path failed - mutex is contended
     * In real kernel, this would call __mutex_lock_slowpath()
     * For this example, we'll just spin-wait (simplified)
     */
    atomic_fetch_add(&fast_path_failure, 1);
    
    unsigned long curr = (unsigned long)pthread_self();
    unsigned long expected;
    
    while (1) {
        expected = 0;
        if (atomic_compare_exchange_weak_explicit(
                &lock->owner, 
                &expected, 
                curr,
                memory_order_acquire,
                memory_order_relaxed)) {
            break;
        }
        /* Brief pause before retry */
        usleep(10);
    }
}

/*
 * mutex_unlock - Release the mutex
 */
void mutex_unlock(mutex_t *lock)
{
    atomic_store_explicit(&lock->owner, 0, memory_order_release);
}

/*
 * Critical section protected by mutex
 */
void critical_section(int thread_id)
{
    mutex_lock(&my_mutex);
    
    /* Critical section */
    printf("[Thread %d] Entering critical section, counter = %d\n", 
           thread_id, shared_counter);
    
    int old_value = shared_counter;
    usleep(1000);  /* Simulate work */
    shared_counter = old_value + 1;
    
    printf("[Thread %d] Exiting critical section, counter = %d\n", 
           thread_id, shared_counter);
    
    mutex_unlock(&my_mutex);
}

void *thread_func(void *arg)
{
    int thread_id = *(int *)arg;
    
    for (int i = 0; i < 3; i++) {
        critical_section(thread_id);
        usleep(100);
    }
    
    return NULL;
}

int main(void)
{
    pthread_t threads[4];
    int thread_ids[4];
    
    printf("=== Mutex Lock Fast Path Demo ===\n");
    printf("Demonstrating optimistic mutex acquisition\n\n");
    
    printf("Fast path uses atomic_compare_exchange:\n");
    printf("  if (owner == 0)  // Mutex is free\n");
    printf("      owner = current_thread;\n");
    printf("      return SUCCESS;\n");
    printf("  else\n");
    printf("      return FAILURE;  // Take slow path\n\n");
    
    /* Create 4 threads */
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
    printf("Fast path successes: %d\n", atomic_load(&fast_path_success));
    printf("Fast path failures: %d\n", atomic_load(&fast_path_failure));
    
    int total = atomic_load(&fast_path_success) + atomic_load(&fast_path_failure);
    if (total > 0) {
        float success_rate = 100.0f * atomic_load(&fast_path_success) / total;
        printf("Fast path success rate: %.1f%%\n", success_rate);
    }
    
    if (shared_counter == 12) {
        printf("\n✓ SUCCESS: Mutex correctly protected shared counter\n");
        return 0;
    } else {
        printf("\n!! ERROR: Counter mismatch\n");
        return 1;
    }
}
