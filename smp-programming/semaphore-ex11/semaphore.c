/*
 * Semaphore Example
 * 
 * Demonstrates counting semaphore implementation.
 * Unlike mutex (binary, 0 or 1), semaphore can have multiple resources.
 * Allows N threads to access resource simultaneously.
 * 
 * Key concepts:
 * - down() / P() / acquire: Decrement counter, block if 0
 * - up() / V() / release: Increment counter, wake waiters
 * - Counting semaphore: Limits concurrent access to N
 * 
 * Based on kernel semaphore from include/linux/semaphore.h
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>
#include <time.h>

/* Semaphore structure */
typedef struct {
    atomic_int count;
    pthread_mutex_t wait_lock;
    pthread_cond_t wait_cond;
} semaphore_t;

/* Statistics */
static atomic_int successful_down = 0;
static atomic_int blocked_down = 0;
static atomic_int total_up = 0;

/*
 * sema_init - Initialize semaphore
 * @sem: semaphore to initialize
 * @val: initial count value
 */
void sema_init(semaphore_t *sem, int val)
{
    atomic_init(&sem->count, val);
    pthread_mutex_init(&sem->wait_lock, NULL);
    pthread_cond_init(&sem->wait_cond, NULL);
}

/*
 * down - Acquire semaphore (P operation)
 * 
 * Decrements count. If count becomes negative, caller sleeps.
 * This is the blocking version.
 */
void down(semaphore_t *sem)
{
    /* Try fast path: decrement if count > 0 */
    int old_count = atomic_load(&sem->count);
    
    while (old_count > 0) {
        if (atomic_compare_exchange_weak(&sem->count, &old_count, old_count - 1)) {
            /* Fast path success */
            atomic_fetch_add(&successful_down, 1);
            return;
        }
        /* CAS failed, retry with updated old_count */
    }
    
    /* Slow path: need to wait */
    atomic_fetch_add(&blocked_down, 1);
    
    pthread_mutex_lock(&sem->wait_lock);
    
    /* Decrement count (might go negative) */
    atomic_fetch_sub(&sem->count, 1);
    
    /* Wait until count becomes positive */
    while (atomic_load(&sem->count) < 0) {
        pthread_cond_wait(&sem->wait_cond, &sem->wait_lock);
    }
    
    pthread_mutex_unlock(&sem->wait_lock);
}

/*
 * down_trylock - Try to acquire semaphore without blocking
 * 
 * Returns 0 on success, -1 if would block.
 */
int down_trylock(semaphore_t *sem)
{
    int old_count = atomic_load(&sem->count);
    
    if (old_count <= 0) {
        return -1;  /* Would block */
    }
    
    if (atomic_compare_exchange_strong(&sem->count, &old_count, old_count - 1)) {
        atomic_fetch_add(&successful_down, 1);
        return 0;  /* Success */
    }
    
    return -1;  /* Failed */
}

/*
 * up - Release semaphore (V operation)
 * 
 * Increments count and wakes one waiter if any.
 */
void up(semaphore_t *sem)
{
    atomic_fetch_add(&total_up, 1);
    
    pthread_mutex_lock(&sem->wait_lock);
    
    /* Increment count */
    atomic_fetch_add(&sem->count, 1);
    
    /* Wake one waiter */
    pthread_cond_signal(&sem->wait_cond);
    
    pthread_mutex_unlock(&sem->wait_lock);
}

/*
 * Demo: Limited resource pool
 * 
 * Simulate 3 printers shared by 6 threads.
 * Only 3 threads can "print" simultaneously.
 */

#define MAX_PRINTERS 3
#define NUM_THREADS 6
#define PRINT_JOBS_PER_THREAD 2

static semaphore_t printer_pool;
static atomic_int active_printers = 0;
static atomic_int total_jobs = 0;

void *printer_thread(void *arg)
{
    int thread_id = *(int *)arg;
    
    for (int job = 1; job <= PRINT_JOBS_PER_THREAD; job++) {
        /* Try to acquire a printer */
        printf("[Thread %d] Job %d: Requesting printer...\n", thread_id, job);
        
        down(&printer_pool);
        
        /* Got a printer! */
        int active = atomic_fetch_add(&active_printers, 1) + 1;
        int job_num = atomic_fetch_add(&total_jobs, 1) + 1;
        
        printf("[Thread %d] Job %d: Got printer! (Active: %d/%d, Job#%d)\n", 
               thread_id, job, active, MAX_PRINTERS, job_num);
        
        /* Simulate printing */
        usleep(500000 + (rand() % 500000));  /* 0.5-1 second */
        
        printf("[Thread %d] Job %d: Printing done, releasing printer\n", 
               thread_id, job);
        
        /* Release printer */
        atomic_fetch_sub(&active_printers, 1);
        up(&printer_pool);
        
        /* Small delay before next job */
        usleep(100000);
    }
    
    return NULL;
}

/*
 * Demo 2: Producer-Consumer with bounded buffer
 */

#define BUFFER_SIZE 5

static semaphore_t empty_slots;  /* Counts empty slots */
static semaphore_t filled_slots; /* Counts filled slots */
static pthread_mutex_t buffer_lock;
static int buffer[BUFFER_SIZE];
static int in = 0;   /* Producer index */
static int out = 0;  /* Consumer index */

void *producer_thread(void *arg)
{
    int id = *(int *)arg;
    
    for (int i = 0; i < 5; i++) {
        int item = id * 100 + i;
        
        /* Wait for empty slot */
        down(&empty_slots);
        
        /* Add item to buffer */
        pthread_mutex_lock(&buffer_lock);
        buffer[in] = item;
        printf("[Producer %d] Produced item %d at index %d\n", id, item, in);
        in = (in + 1) % BUFFER_SIZE;
        pthread_mutex_unlock(&buffer_lock);
        
        /* Signal filled slot */
        up(&filled_slots);
        
        usleep(200000);  /* 200ms */
    }
    
    return NULL;
}

void *consumer_thread(void *arg)
{
    int id = *(int *)arg;
    
    for (int i = 0; i < 5; i++) {
        /* Wait for filled slot */
        down(&filled_slots);
        
        /* Remove item from buffer */
        pthread_mutex_lock(&buffer_lock);
        int item = buffer[out];
        printf("[Consumer %d] Consumed item %d from index %d\n", id, item, out);
        out = (out + 1) % BUFFER_SIZE;
        pthread_mutex_unlock(&buffer_lock);
        
        /* Signal empty slot */
        up(&empty_slots);
        
        usleep(300000);  /* 300ms */
    }
    
    return NULL;
}

int main(void)
{
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    
    srand(time(NULL));
    
    printf("=== Semaphore Demo ===\n\n");
    
    /* Demo 1: Limited resource pool */
    printf("Demo 1: Printer Pool (3 printers, 6 threads)\n");
    printf("==============================================\n\n");
    
    sema_init(&printer_pool, MAX_PRINTERS);
    
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i + 1;
        pthread_create(&threads[i], NULL, printer_thread, &thread_ids[i]);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\n--- Printer Pool Results ---\n");
    printf("Total jobs completed: %d\n", atomic_load(&total_jobs));
    printf("Fast path acquisitions: %d\n", atomic_load(&successful_down));
    printf("Slow path (blocked): %d\n", atomic_load(&blocked_down));
    printf("Total releases: %d\n\n", atomic_load(&total_up));
    
    /* Demo 2: Producer-Consumer */
    printf("\nDemo 2: Producer-Consumer (Buffer size: %d)\n", BUFFER_SIZE);
    printf("==============================================\n\n");
    
    sema_init(&empty_slots, BUFFER_SIZE);  /* All slots empty initially */
    sema_init(&filled_slots, 0);           /* No filled slots initially */
    pthread_mutex_init(&buffer_lock, NULL);
    
    pthread_t producers[2], consumers[2];
    int prod_ids[2] = {1, 2};
    int cons_ids[2] = {1, 2};
    
    for (int i = 0; i < 2; i++) {
        pthread_create(&producers[i], NULL, producer_thread, &prod_ids[i]);
        pthread_create(&consumers[i], NULL, consumer_thread, &cons_ids[i]);
    }
    
    for (int i = 0; i < 2; i++) {
        pthread_join(producers[i], NULL);
        pthread_join(consumers[i], NULL);
    }
    
    printf("\n--- Producer-Consumer Results ---\n");
    printf("Buffer empty slots: %d\n", atomic_load(&empty_slots.count));
    printf("Buffer filled slots: %d\n", atomic_load(&filled_slots.count));
    
    printf("\n✓ Semaphore operations completed successfully\n");
    return 0;
}
