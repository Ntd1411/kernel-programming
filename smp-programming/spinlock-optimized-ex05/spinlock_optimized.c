/*
 * Optimized Spin Lock Example
 * 
 * Demonstrates optimized spin lock implementation that reduces cache thrashing
 * using the read-first optimization and PAUSE instruction.
 * 
 * Key optimization: Test lock with non-atomic read before attempting atomic acquire.
 * This keeps the cache line in shared state while spinning, reducing invalidations.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>
#include <time.h>

/* Spin lock structure */
typedef struct {
    atomic_int lock;
} spinlock_t;

/* Shared counter protected by spin lock */
static int shared_counter = 0;
static spinlock_t my_lock = { 0 };

/* Statistics */
static atomic_long total_acquisitions = 0;
static atomic_long total_spins = 0;
static atomic_long total_pauses = 0;

/*
 * cpu_pause - Simulate PAUSE instruction
 * 
 * On x86, PAUSE instruction:
 * - Hints to CPU that we're in a spin-wait loop
 * - Reduces power consumption
 * - Avoids pipeline flushes from memory order violations
 */
static inline void cpu_pause(void)
{
    /* On x86: asm volatile("pause" ::: "memory"); */
    /* User-space simulation: just a hint to scheduler */
    #ifdef __x86_64__
    __asm__ __volatile__("pause" ::: "memory");
    #else
    /* For other architectures, yield */
    sched_yield();
    #endif
    
    atomic_fetch_add(&total_pauses, 1);
}

/*
 * Naive spin lock (for comparison) - causes cache thrashing
 */
void spin_lock_naive(spinlock_t *lock)
{
    int expected;
    int spin_count = 0;
    
    do {
        expected = 0;
        if (spin_count > 0) {
            cpu_pause();
        }
        spin_count++;
    } while (!atomic_compare_exchange_weak(&lock->lock, &expected, 1));
    
    atomic_fetch_add(&total_acquisitions, 1);
    atomic_fetch_add(&total_spins, spin_count - 1);
}

/*
 * Optimized spin lock - reduces cache thrashing
 * 
 * Mimics x86 assembly:
 *   spin_lock:
 *       rep ; nop           ; PAUSE instruction
 *       test lock_addr, 1   ; Non-atomic read-only test
 *       jnz spin_lock       ; Keep spinning if locked
 *       lock bts lock_addr  ; Only try to acquire when it looks free
 *       jc spin_lock
 * 
 * Key insight: Non-atomic reads keep cache line in SHARED state.
 * Only attempt atomic write when lock appears free.
 */
void spin_lock_optimized(spinlock_t *lock)
{
    int expected;
    int spin_count = 0;
    
    while (1) {
        /* Read-only test - keeps cache line shared */
        while (atomic_load_explicit(&lock->lock, memory_order_relaxed) != 0) {
            cpu_pause();
            spin_count++;
        }
        
        /* Lock looks free, try to acquire with atomic operation */
        expected = 0;
        if (atomic_compare_exchange_weak_explicit(
                &lock->lock, &expected, 1,
                memory_order_acquire, memory_order_relaxed)) {
            /* Success! */
            break;
        }
        
        /* Failed - someone else got it first, back to spinning */
        spin_count++;
    }
    
    atomic_fetch_add(&total_acquisitions, 1);
    atomic_fetch_add(&total_spins, spin_count);
}

/*
 * spin_unlock - Release the lock
 */
void spin_unlock(spinlock_t *lock)
{
    atomic_store_explicit(&lock->lock, 0, memory_order_release);
}

/* Global flag to switch between naive and optimized implementations */
static int use_optimized = 1;

/*
 * Critical section protected by spin lock
 */
void critical_section(int thread_id)
{
    if (use_optimized) {
        spin_lock_optimized(&my_lock);
    } else {
        spin_lock_naive(&my_lock);
    }
    
    /* Critical section */
    int old_value = shared_counter;
    usleep(100);  /* Simulate work - increases contention */
    shared_counter = old_value + 1;
    
    spin_unlock(&my_lock);
}

void *thread_func(void *arg)
{
    int thread_id = *(int *)arg;
    
    for (int i = 0; i < 10; i++) {
        critical_section(thread_id);
        usleep(10);
    }
    
    return NULL;
}

void run_test(const char *test_name, int optimized)
{
    pthread_t threads[4];
    int thread_ids[4];
    struct timespec start, end;
    
    /* Reset state */
    shared_counter = 0;
    atomic_store(&my_lock.lock, 0);
    atomic_store(&total_acquisitions, 0);
    atomic_store(&total_spins, 0);
    atomic_store(&total_pauses, 0);
    use_optimized = optimized;
    
    printf("\n=== %s ===\n", test_name);
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    /* Create 4 threads */
    for (int i = 0; i < 4; i++) {
        thread_ids[i] = i + 1;
        pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]);
    }
    
    /* Wait for completion */
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    long elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000L + 
                      (end.tv_nsec - start.tv_nsec);
    
    /* Display results */
    printf("Final counter: %d (expected: 40)\n", shared_counter);
    printf("Total acquisitions: %ld\n", atomic_load(&total_acquisitions));
    printf("Total spins: %ld\n", atomic_load(&total_spins));
    printf("Total PAUSE instructions: %ld\n", atomic_load(&total_pauses));
    printf("Elapsed time: %.2f ms\n", elapsed_ns / 1000000.0);
    
    if (atomic_load(&total_acquisitions) > 0) {
        float avg_spins = (float)atomic_load(&total_spins) / 
                         atomic_load(&total_acquisitions);
        printf("Average spins per acquisition: %.2f\n", avg_spins);
    }
}

int main(void)
{
    printf("=== Optimized Spin Lock Demo ===\n");
    printf("Demonstrating cache-friendly spin lock optimization\n");
    
    /* Run naive version */
    run_test("Naive Spin Lock (Cache Thrashing)", 0);
    
    /* Run optimized version */
    run_test("Optimized Spin Lock (Read-First)", 1);
    
    printf("\n=== Comparison ===\n");
    printf("The optimized version should show:\n");
    printf("  - Similar or lower total spins\n");
    printf("  - Better performance (lower elapsed time)\n");
    printf("  - Less cache thrashing (in real kernel)\n");
    
    printf("\n✓ Demo complete\n");
    return 0;
}
