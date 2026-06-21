/*
 * IRQ Disable Example
 * 
 * Demonstrates interrupt disabling/enabling mechanisms for synchronization.
 * 
 * Note: On user-space, we can't actually disable hardware interrupts.
 * This example simulates the concept using signals and signal masking.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

/* Shared counter */
static int counter = 0;
static pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;

/* Signal handler simulating an interrupt */
void signal_handler(int signum)
{
    printf("[IRQ] Interrupt received (signal %d), counter = %d\n", signum, counter);
}

/*
 * Simulate local_irq_disable() - block signals (simulated interrupts)
 */
void local_irq_disable(void)
{
    sigset_t set;
    sigfillset(&set);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    printf("[IRQ] Interrupts disabled\n");
}

/*
 * Simulate local_irq_enable() - unblock signals
 */
void local_irq_enable(void)
{
    sigset_t set;
    sigfillset(&set);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);
    printf("[IRQ] Interrupts enabled\n");
}

/*
 * Simulate local_irq_save() - save current state and disable
 */
void local_irq_save(sigset_t *flags)
{
    sigset_t set;
    sigfillset(&set);
    pthread_sigmask(SIG_BLOCK, &set, flags);
    printf("[IRQ] Interrupts saved and disabled\n");
}

/*
 * Simulate local_irq_restore() - restore previously saved state
 */
void local_irq_restore(sigset_t *flags)
{
    pthread_sigmask(SIG_SETMASK, flags, NULL);
    printf("[IRQ] Interrupts restored\n");
}

/*
 * Critical section with interrupt disabling
 */
void critical_section_with_irq_disable(void)
{
    printf("\n=== Method 1: local_irq_disable/enable ===\n");
    
    local_irq_disable();
    
    /* Critical section - interrupts are disabled */
    printf("[CRITICAL] Modifying counter: %d -> ", counter);
    counter += 10;
    printf("%d\n", counter);
    usleep(100000);  /* 100ms - simulating work */
    
    local_irq_enable();
}

/*
 * Critical section with interrupt save/restore
 */
void critical_section_with_irq_save(void)
{
    sigset_t flags;
    
    printf("\n=== Method 2: local_irq_save/restore ===\n");
    
    local_irq_save(&flags);
    
    /* Critical section - interrupts are disabled */
    printf("[CRITICAL] Modifying counter: %d -> ", counter);
    counter += 20;
    printf("%d\n", counter);
    usleep(100000);  /* 100ms - simulating work */
    
    local_irq_restore(&flags);
}

/*
 * Nested critical section demonstrating why save/restore is preferred
 */
void nested_critical_section(void)
{
    sigset_t flags;
    
    printf("\n=== Method 3: Nested critical sections ===\n");
    
    local_irq_save(&flags);
    printf("[OUTER] Outer critical section start\n");
    
    /* Inner critical section */
    {
        sigset_t inner_flags;
        local_irq_save(&inner_flags);
        printf("[INNER] Inner critical section\n");
        counter += 5;
        usleep(50000);
        local_irq_restore(&inner_flags);
    }
    
    printf("[OUTER] Outer critical section end\n");
    local_irq_restore(&flags);
}

void *sender_thread(void *arg)
{
    pthread_t tid = pthread_self();
    
    printf("\n[Thread] Sending signals to simulate interrupts...\n");
    sleep(1);
    
    for (int i = 0; i < 3; i++) {
        pthread_kill(tid, SIGUSR1);
        usleep(50000);
    }
    
    return NULL;
}

int main(void)
{
    struct sigaction sa;
    
    printf("=== IRQ Disable/Enable Demo ===\n");
    printf("Simulating interrupt disabling using signal masking\n\n");
    
    /* Set up signal handler */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigaction(SIGUSR1, &sa, NULL);
    
    printf("Initial counter: %d\n", counter);
    
    /* Demo 1: Basic disable/enable */
    critical_section_with_irq_disable();
    
    /* Demo 2: Save/restore */
    critical_section_with_irq_save();
    
    /* Demo 3: Nested critical sections */
    nested_critical_section();
    
    printf("\n=== Results ===\n");
    printf("Final counter: %d\n", counter);
    printf("\n✓ All critical sections executed safely\n");
    
    return 0;
}
