/*
 * Basic Spin Lock Example
 * 
 * Demonstrates basic spin lock implementation using atomic operations.
 * 
 * This example shows the fundamental spin lock mechanism with test-and-set
 * (similar to x86 'lock bts' instruction behavior).
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>

/* Simple spin lock using atomic flag */
typedef struct {
    atomic_flag lock;
} spinlock_t;

/* Shared counter protected by spin lock */
static int shared_counter = 0;
static spinlock_t my_lock = { ATOMIC_FLAG_INIT };

/* Statistics */
static atomic_int total_acquisitions = 0;
static atomic_int total_spins = 0;

/*
 * spin_lock - Acquire the spin lock
 * 
 * Mimics x86 assembly:
 *   spin_lock:
 *       lock bts [my_lock], 0   ; atomic bit-test-and-set
 *       jc spin_lock            ; spin if carry (lock was taken)
 * 
 * The atomic_flag_test_and_set returns true if lock was already set.
 */
void spin_lock(spinlock_t *lock)
{
    int spin_count = 0;
    
    while (atomic_flag_test_and_set_explicit(&lock->lock, memory_order_acquire)) {
        /* Lock was already held, spin */
        spin_count++;
    }
    
    atomic_fetch_add(&total_acquisitions, 1);
    atomic_fetch_add(&total_spins, spin_count);
}

/*
 * spin_unlock - Release the spin lock
 * 
 * Mimics x86 assembly:
 *   spin_unlock:
 *       mov [my_lock], 0
 */
void spin_unlock(spinlock_t *lock)
{
    atomic_flag_clear_explicit(&lock->lock, memory_order_release);
}

/*
 * Critical section protected by spin lock
 */
void critical_section(int thread_id)
{
    spin_lock(&my_lock);
    
    /* Critical section - only one thread can be here at a time */
    printf("[Thread %d] Entering critical section, counter = %d\n", 
           thread_id, shared_counter);
    
    int old_value = shared_counter;
    usleep(1000);  /* Simulate work - increases contention */
    shared_counter = old_value + 1;
    
    printf("[Thread %d] Exiting critical section, counter = %d\n", 
           thread_id, shared_counter);
    
    spin_unlock(&my_lock);
}

void *thread_func(void *arg)
{
    int thread_id = *(int *)arg;
    
    for (int i = 0; i < 5; i++) {
        critical_section(thread_id);
        usleep(100);  /* Small delay between acquisitions */
    }
    
    return NULL;
}

/*
 * Demonstrate the bts (bit test and set) instruction behavior
 */
void demonstrate_bts(void)
{
    printf("\n=== BTS Instruction Behavior ===\n");
    printf("bts dts, src - bit test and set\n");
    printf("  CF <- dts[src]    ; Copy bit to carry flag\n");
    printf("  dts[src] <- 1     ; Set the bit\n\n");
    
    printf("Example:\n");
    printf("  Initial: lock = 0\n");
    printf("  Thread A: bts [lock], 0 -> CF=0, lock=1 (acquired)\n");
    printf("  Thread B: bts [lock], 0 -> CF=1, lock=1 (failed, must spin)\n");
    printf("  Thread A: mov [lock], 0 -> lock=0 (released)\n");
    printf("  Thread B: bts [lock], 0 -> CF=0, lock=1 (acquired)\n");
}

int main(void)
{
    pthread_t threads[4];
    int thread_ids[4];
    
    printf("=== Basic Spin Lock Demo ===\n");
    demonstrate_bts();
    
    printf("\n=== Running Concurrent Test ===\n");
    printf("Creating 4 threads, each incrementing counter 5 times...\n\n");
    
    /* Create 4 threads */
    for (int i = 0; i < 4; i++) {
        thread_ids[i] = i + 1;
        pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]);
    }
    
    /* Wait for all threads to complete */
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }
    
    /* Display results */
    printf("\n=== Results ===\n");
    printf("Final counter: %d (expected: 20)\n", shared_counter);
    printf("Total lock acquisitions: %d\n", atomic_load(&total_acquisitions));
    printf("Total spins: %d\n", atomic_load(&total_spins));
    
    if (atomic_load(&total_spins) > 0) {
        float avg_spins = (float)atomic_load(&total_spins) / atomic_load(&total_acquisitions);
        printf("Average spins per acquisition: %.2f\n", avg_spins);
    }
    
    if (shared_counter == 20) {
        printf("\n✓ SUCCESS: Counter is correct (no race condition)\n");
        return 0;
    } else {
        printf("\n!! ERROR: Counter is incorrect (expected 20, got %d)\n", shared_counter);
        return 1;
    }
}
