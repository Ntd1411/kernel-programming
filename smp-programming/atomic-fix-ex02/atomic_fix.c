/*
 * Atomic Fix Example
 * 
 * Demonstrates the correct implementation of release_resource()
 * using atomic operations to prevent race conditions.
 * 
 * This is the FIXED version using atomic_dec_and_test approach.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

/* Atomic resource counter */
static atomic_int counter = 10;
static int resource_freed = 0;
static pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;

/* Simulated resource */
void free_resource(void)
{
    pthread_mutex_lock(&print_lock);
    resource_freed++;
    printf("[FREED] Resource freed (total frees: %d)\n", resource_freed);
    pthread_mutex_unlock(&print_lock);
}

/*
 * CORRECT IMPLEMENTATION
 * Uses atomic_fetch_sub which atomically:
 * 1. Fetches the current value
 * 2. Decrements it
 * 3. Returns the OLD value
 * 
 * This mimics kernel's atomic_dec_and_test behavior
 */
void release_resource(void)
{
    int old_value = atomic_fetch_sub(&counter, 1);
    
    /* If old value was 1, we just decremented to 0 */
    if (old_value == 1) {
        free_resource();
    }
}

void *thread_func(void *arg)
{
    int thread_id = *(int *)arg;
    
    pthread_mutex_lock(&print_lock);
    printf("[Thread %d] Releasing resource...\n", thread_id);
    pthread_mutex_unlock(&print_lock);
    
    release_resource();
    return NULL;
}

int main(void)
{
    pthread_t threads[10];
    int thread_ids[10];
    
    printf("=== Atomic Fix Demo ===\n");
    printf("Initial counter: %d\n\n", atomic_load(&counter));
    
    /* Create 10 threads, each calling release_resource() */
    for (int i = 0; i < 10; i++) {
        thread_ids[i] = i + 1;
        pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]);
    }
    
    /* Wait for all threads to complete */
    for (int i = 0; i < 10; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\n=== Results ===\n");
    printf("Final counter: %d\n", atomic_load(&counter));
    printf("Resource freed: %d times\n", resource_freed);
    
    if (resource_freed == 1) {
        printf("\n✓ SUCCESS: Resource freed exactly once (no race condition)\n");
        return 0;
    } else {
        printf("\n!! ERROR: Resource freed %d times (expected 1)\n", resource_freed);
        return 1;
    }
}
