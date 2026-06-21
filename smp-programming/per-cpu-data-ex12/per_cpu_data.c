/*
 * Per-CPU Data Example
 * 
 * Demonstrates per-CPU variables optimization.
 * Each CPU has its own copy of data, avoiding cache line bouncing.
 * 
 * Key concepts:
 * - Per-CPU allocation: Each CPU has separate copy
 * - No contention: Local access without locks
 * - Cache friendly: Each CPU works on its own cache line
 * - Aggregation: Sum all CPU values for global view
 * 
 * Based on kernel per-CPU API from include/linux/percpu.h
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>

#define MAX_CPUS 8
#define NUM_THREADS 8
#define ITERATIONS 1000000

/*
 * Per-CPU data structure
 * Each CPU has its own instance of this structure
 */
typedef struct {
    long counter;
    long operations;
    char padding[64 - 2*sizeof(long)]; /* Pad to cache line */
} percpu_data_t;

/* Per-CPU data array - one entry per CPU */
static percpu_data_t percpu_counters[MAX_CPUS] __attribute__((aligned(64)));

/* Shared counter for comparison */
static atomic_long shared_counter = 0;
static atomic_long shared_operations = 0;

/* Statistics */
static atomic_long percpu_time_ns = 0;
static atomic_long shared_time_ns = 0;

/*
 * get_cpu - Disable preemption and get current CPU
 * 
 * In kernel, this prevents migration to another CPU.
 * In userspace, we use sched_getcpu() and thread affinity.
 */
static inline int get_cpu(void)
{
    return sched_getcpu();
}

/*
 * put_cpu - Re-enable preemption
 * 
 * In kernel, allows thread to be migrated again.
 * In userspace, this is a no-op (we use affinity instead).
 */
static inline void put_cpu(void)
{
    /* No-op in userspace */
}

/*
 * per_cpu_ptr - Get pointer to per-CPU data
 * @data: base address of per-CPU data array
 * @cpu: CPU number
 */
static inline percpu_data_t *per_cpu_ptr(percpu_data_t *data, int cpu)
{
    return &data[cpu];
}

/*
 * this_cpu_add - Add to per-CPU counter (fast, no lock)
 * 
 * In kernel: Direct access to CPU-local data without atomic ops
 * In userspace: We simulate with array indexed by CPU ID
 */
static void this_cpu_add_counter(long val)
{
    int cpu = get_cpu();
    percpu_data_t *ptr = per_cpu_ptr(percpu_counters, cpu);
    
    /* Fast: direct access, no atomics needed */
    ptr->counter += val;
    ptr->operations++;
    
    put_cpu();
}

/*
 * Aggregate all per-CPU counters
 */
static long percpu_counter_sum(void)
{
    long sum = 0;
    
    for (int cpu = 0; cpu < MAX_CPUS; cpu++) {
        sum += percpu_counters[cpu].counter;
    }
    
    return sum;
}

/*
 * Get total operations across all CPUs
 */
static long percpu_operations_sum(void)
{
    long sum = 0;
    
    for (int cpu = 0; cpu < MAX_CPUS; cpu++) {
        sum += percpu_counters[cpu].operations;
    }
    
    return sum;
}

/*
 * Shared counter operations (for comparison)
 */
static void shared_add_counter(long val)
{
    /* Slow: atomic operation, cache line bouncing */
    atomic_fetch_add(&shared_counter, val);
    atomic_fetch_add(&shared_operations, 1);
}

/*
 * Get nanosecond timestamp
 */
static long get_time_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000L + ts.tv_nsec;
}

/*
 * Worker thread for per-CPU counter test
 */
void *percpu_worker(void *arg)
{
    int thread_id = *(int *)arg;
    
    /* Pin thread to CPU */
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(thread_id % MAX_CPUS, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    
    long start = get_time_ns();
    
    for (int i = 0; i < ITERATIONS; i++) {
        this_cpu_add_counter(1);
    }
    
    long elapsed = get_time_ns() - start;
    atomic_fetch_add(&percpu_time_ns, elapsed);
    
    return NULL;
}

/*
 * Worker thread for shared counter test
 */
void *shared_worker(void *arg)
{
    int thread_id = *(int *)arg;
    
    /* Pin thread to CPU */
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(thread_id % MAX_CPUS, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    
    long start = get_time_ns();
    
    for (int i = 0; i < ITERATIONS; i++) {
        shared_add_counter(1);
    }
    
    long elapsed = get_time_ns() - start;
    atomic_fetch_add(&shared_time_ns, elapsed);
    
    return NULL;
}

int main(void)
{
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    
    printf("=== Per-CPU Data Demo ===\n\n");
    printf("Configuration:\n");
    printf("  Threads: %d\n", NUM_THREADS);
    printf("  CPUs available: %d\n", MAX_CPUS);
    printf("  Iterations per thread: %d\n", ITERATIONS);
    printf("  Total operations: %d\n\n", NUM_THREADS * ITERATIONS);
    
    /* Test 1: Per-CPU counters */
    printf("Test 1: Per-CPU Counters (No Contention)\n");
    printf("==========================================\n");
    
    /* Initialize per-CPU data */
    for (int i = 0; i < MAX_CPUS; i++) {
        percpu_counters[i].counter = 0;
        percpu_counters[i].operations = 0;
    }
    
    atomic_store(&percpu_time_ns, 0);
    
    long test1_start = get_time_ns();
    
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, percpu_worker, &thread_ids[i]);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    long test1_time = get_time_ns() - test1_start;
    
    printf("Total time: %.2f ms\n", test1_time / 1000000.0);
    printf("Average per thread: %.2f ms\n", atomic_load(&percpu_time_ns) / NUM_THREADS / 1000000.0);
    printf("Final counter sum: %ld\n", percpu_counter_sum());
    printf("Total operations: %ld\n", percpu_operations_sum());
    
    /* Show per-CPU distribution */
    printf("\nPer-CPU distribution:\n");
    for (int i = 0; i < MAX_CPUS; i++) {
        if (percpu_counters[i].operations > 0) {
            printf("  CPU %d: counter=%ld, ops=%ld\n", 
                   i, percpu_counters[i].counter, percpu_counters[i].operations);
        }
    }
    
    /* Test 2: Shared counter (for comparison) */
    printf("\n\nTest 2: Shared Counter (With Contention)\n");
    printf("=========================================\n");
    
    atomic_store(&shared_counter, 0);
    atomic_store(&shared_operations, 0);
    atomic_store(&shared_time_ns, 0);
    
    long test2_start = get_time_ns();
    
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, shared_worker, &thread_ids[i]);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    long test2_time = get_time_ns() - test2_start;
    
    printf("Total time: %.2f ms\n", test2_time / 1000000.0);
    printf("Average per thread: %.2f ms\n", atomic_load(&shared_time_ns) / NUM_THREADS / 1000000.0);
    printf("Final counter: %ld\n", atomic_load(&shared_counter));
    printf("Total operations: %ld\n", atomic_load(&shared_operations));
    
    /* Performance comparison */
    printf("\n\n=== Performance Comparison ===\n");
    printf("Per-CPU: %.2f ms\n", test1_time / 1000000.0);
    printf("Shared:  %.2f ms\n", test2_time / 1000000.0);
    
    double speedup = (double)test2_time / test1_time;
    printf("Speedup: %.2fx faster with per-CPU\n", speedup);
    
    if (speedup > 1.0) {
        printf("\n✓ Per-CPU optimization successful!\n");
        printf("  Benefit: %.0f%% reduction in execution time\n", (speedup - 1.0) * 100.0);
    }
    
    return 0;
}
