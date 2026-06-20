/*
 * Race Condition Example
 * 
 * Demonstrates the classic race condition in release_resource()
 * where multiple threads can incorrectly free a resource multiple times.
 * 
 * This is the BUGGY version - DO NOT use in production!
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

/* Global resource counter */
static int counter = 10;
static int resource_freed = 0;

/* Simulated resource */
void free_resource(void)
{
    resource_freed++;
    printf("[FREED] Resource freed (total frees: %d)\n", resource_freed);
}

/*
 * BUGGY IMPLEMENTATION
 * This function has a race condition:
 * - Thread A decrements counter to 1
 * - Thread A is preempted
 * - Thread B decrements counter to 0 and frees resource
 * - Thread A resumes, sees counter as 0, frees resource AGAIN
 */
void release_resource(void)
{
    counter--;
    
    /* Small sleep to increase chance of race condition */
    usleep(1);
    
    if (!counter)
        free_resource();
}

void *thread_func(void *arg)
{
    int thread_id = *(int *)arg;
    printf("[Thread %d] Releasing resource...\n", thread_id);
    release_resource();
    return NULL;
}

int main(void)
{
    pthread_t threads[10];
    int thread_ids[10];
    
    printf("=== Race Condition Demo ===\n");
    printf("Initial counter: %d\n\n", counter);
    
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
    printf("Final counter: %d\n", counter);
    printf("Resource freed: %d times\n", resource_freed);
    
    if (resource_freed > 1) {
        printf("\n!! BUG DETECTED: Resource freed multiple times!\n");
        printf("This is a race condition - the resource should only be freed once.\n");
        return 1;
    } else {
        printf("\n✓ No race detected in this run (try running multiple times)\n");
        return 0;
    }
}
