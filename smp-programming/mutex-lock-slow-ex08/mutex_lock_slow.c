/*
 * Mutex Lock Slow Path Example
 * 
 * Demonstrates the slow path of mutex_lock() where threads must sleep
 * when the mutex is contended. Simulates wait queue management.
 * 
 * Based on kernel implementation from kernel/locking/mutex.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>
#include <time.h>

/* Simplified waiter structure */
struct mutex_waiter {
    pthread_t task;
    struct mutex_waiter *next;
};

/* Simplified mutex structure with wait list */
typedef struct {
    atomic_ulong owner;
    pthread_mutex_t wait_lock;  /* Protects wait_list */
    struct mutex_waiter *wait_list;
} mutex_t;

/* Statistics */
static atomic_int fast_path_count = 0;
static atomic_int slow_path_count = 0;
static atomic_int total_wait_time_ms = 0;

/* Shared counter */
static int shared_counter = 0;
static mutex_t my_mutex = { 0, PTHREAD_MUTEX_INITIALIZER, NULL };

/* Helper: Add waiter to end of list */
static void list_add_tail(struct mutex_waiter **head, struct mutex_waiter *waiter)
{
    if (*head == NULL) {
        *head = waiter;
        waiter->next = NULL;
    } else {
        struct mutex_waiter *curr = *head;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = waiter;
        waiter->next = NULL;
    }
}

/* Helper: Remove waiter from list */
static void list_remove(struct mutex_waiter **head, struct mutex_waiter *waiter)
{
    if (*head == waiter) {
        *head = waiter->next;
    } else {
        struct mutex_waiter *curr = *head;
        while (curr && curr->next != waiter) {
            curr = curr->next;
        }
        if (curr) {
            curr->next = waiter->next;
        }
    }
}

/*
 * __mutex_trylock - Try to acquire mutex
 * Returns 1 if successful, 0 otherwise
 */
static inline int __mutex_trylock(mutex_t *lock)
{
    unsigned long curr = (unsigned long)pthread_self();
    unsigned long expected = 0;
    
    return atomic_compare_exchange_strong_explicit(
        &lock->owner,
        &expected,
        curr,
        memory_order_acquire,
        memory_order_relaxed);
}

/*
 * __mutex_lock_slowpath - Slow path when mutex is contended
 * 
 * Algorithm:
 * 1. Acquire wait_lock to protect wait_list
 * 2. Add ourselves to wait_list
 * 3. Release wait_lock and sleep
 * 4. When woken, try to acquire mutex
 * 5. Repeat until successful
 * 6. Remove ourselves from wait_list
 */
static void __mutex_lock_slowpath(mutex_t *lock)
{
    struct mutex_waiter waiter;
    struct timespec start, end;
    
    waiter.task = pthread_self();
    waiter.next = NULL;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    /* Acquire wait_lock to modify wait_list */
    pthread_mutex_lock(&lock->wait_lock);
    
    /* Add ourselves to wait_list */
    list_add_tail(&lock->wait_list, &waiter);
    
    pthread_mutex_unlock(&lock->wait_lock);
    
    /* Sleep and retry loop */
    while (1) {
        /* Try to acquire mutex */
        if (__mutex_trylock(lock)) {
            /* Success! */
            break;
        }
        
        /* Failed - sleep a bit before retrying */
        usleep(1000);  /* Simulate scheduler putting us to sleep */
    }
    
    /* Remove ourselves from wait_list */
    pthread_mutex_lock(&lock->wait_lock);
    list_remove(&lock->wait_list, &waiter);
    pthread_mutex_unlock(&lock->wait_lock);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    long wait_time_ms = (end.tv_sec - start.tv_sec) * 1000 + 
                        (end.tv_nsec - start.tv_nsec) / 1000000;
    atomic_fetch_add(&total_wait_time_ms, (int)wait_time_ms);
}

/*
 * mutex_lock - Acquire the mutex
 * 
 * Fast path: Try atomic acquisition
 * Slow path: Sleep until mutex is available
 */
void mutex_lock(mutex_t *lock)
{
    /* Try fast path first */
    if (__mutex_trylock(lock)) {
        atomic_fetch_add(&fast_path_count, 1);
        return;
    }
    
    /* Fast path failed - take slow path */
    atomic_fetch_add(&slow_path_count, 1);
    __mutex_lock_slowpath(lock);
}

/*
 * mutex_unlock - Release the mutex
 * 
 * Simplified version - just clears owner
 * Real kernel would wake up first waiter
 */
void mutex_unlock(mutex_t *lock)
{
    atomic_store_explicit(&lock->owner, 0, memory_order_release);
}

/*
 * Critical section protected by mutex
 */
void critical_section(int thread_id, int iteration)
{
    mutex_lock(&my_mutex);
    
    /* Critical section */
    printf("[Thread %d:%d] Counter: %d -> ", thread_id, iteration, shared_counter);
    int old_value = shared_counter;
    usleep(2000);  /* Long work - increases contention */
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
    
    printf("=== Mutex Lock Slow Path Demo ===\n");
    printf("Demonstrating wait queue and sleeping behavior\n\n");
    
    printf("Slow path algorithm:\n");
    printf("  1. Acquire wait_lock\n");
    printf("  2. Add waiter to wait_list\n");
    printf("  3. Release wait_lock\n");
    printf("  4. Sleep until mutex available\n");
    printf("  5. Try to acquire mutex\n");
    printf("  6. Repeat until success\n");
    printf("  7. Remove from wait_list\n\n");
    
    /* Create 4 threads with high contention */
    printf("Creating 4 threads with high contention...\n\n");
    
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
    printf("Fast path acquisitions: %d\n", atomic_load(&fast_path_count));
    printf("Slow path acquisitions: %d\n", atomic_load(&slow_path_count));
    printf("Total wait time: %d ms\n", atomic_load(&total_wait_time_ms));
    
    int total = atomic_load(&fast_path_count) + atomic_load(&slow_path_count);
    if (total > 0) {
        float slow_rate = 100.0f * atomic_load(&slow_path_count) / total;
        printf("Slow path rate: %.1f%%\n", slow_rate);
        
        if (atomic_load(&slow_path_count) > 0) {
            float avg_wait = (float)atomic_load(&total_wait_time_ms) / 
                            atomic_load(&slow_path_count);
            printf("Average wait time: %.1f ms\n", avg_wait);
        }
    }
    
    if (shared_counter == 12) {
        printf("\n✓ SUCCESS: Mutex slow path correctly handled contention\n");
        return 0;
    } else {
        printf("\n⚠️  ERROR: Counter mismatch\n");
        return 1;
    }
}
