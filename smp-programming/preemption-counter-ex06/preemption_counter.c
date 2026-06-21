/*
 * Preemption Counter Example
 * 
 * Demonstrates the kernel's preemption counter mechanism used to track
 * and control preemption state, including softirq and hardirq contexts.
 * 
 * This is a user-space simulation of the kernel's preempt_count mechanism.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>

/* 
 * Preemption counter bit layout (from kernel)
 * 
 * Bits 0-7:   Preemption count (0-255)
 * Bits 8-15:  Softirq count (0-255)
 * Bits 16-19: Hardirq count (0-15)
 * Bit  20:    NMI count (0-1)
 */
#define PREEMPT_BITS    8
#define SOFTIRQ_BITS    8
#define HARDIRQ_BITS    4
#define NMI_BITS        1

#define PREEMPT_SHIFT   0
#define SOFTIRQ_SHIFT   (PREEMPT_SHIFT + PREEMPT_BITS)
#define HARDIRQ_SHIFT   (SOFTIRQ_SHIFT + SOFTIRQ_BITS)
#define NMI_SHIFT       (HARDIRQ_SHIFT + HARDIRQ_BITS)

#define PREEMPT_MASK    (((1UL << PREEMPT_BITS) - 1) << PREEMPT_SHIFT)
#define SOFTIRQ_MASK    (((1UL << SOFTIRQ_BITS) - 1) << SOFTIRQ_SHIFT)
#define HARDIRQ_MASK    (((1UL << HARDIRQ_BITS) - 1) << HARDIRQ_SHIFT)
#define NMI_MASK        (((1UL << NMI_BITS) - 1) << NMI_SHIFT)

#define PREEMPT_OFFSET  (1UL << PREEMPT_SHIFT)
#define SOFTIRQ_OFFSET  (1UL << SOFTIRQ_SHIFT)
#define HARDIRQ_OFFSET  (1UL << HARDIRQ_SHIFT)
#define NMI_OFFSET      (1UL << NMI_SHIFT)

/* Per-thread preemption counter */
static __thread unsigned long preempt_count_value = 0;

/* Accessors */
static inline unsigned long preempt_count(void)
{
    return preempt_count_value;
}

static inline void add_preempt_count(unsigned long val)
{
    preempt_count_value += val;
}

static inline void sub_preempt_count(unsigned long val)
{
    preempt_count_value -= val;
}

/* Preemption control */
#define preempt_disable()   add_preempt_count(PREEMPT_OFFSET)
#define preempt_enable()    sub_preempt_count(PREEMPT_OFFSET)

/* Softirq control */
#define local_bh_disable()  add_preempt_count(SOFTIRQ_OFFSET)
#define local_bh_enable()   sub_preempt_count(SOFTIRQ_OFFSET)

/* Context checking */
#define hardirq_count()     (preempt_count() & HARDIRQ_MASK)
#define softirq_count()     (preempt_count() & SOFTIRQ_MASK)
#define irq_count()         (preempt_count() & (HARDIRQ_MASK | SOFTIRQ_MASK))
#define in_interrupt()      (irq_count() != 0)
#define in_softirq()        (softirq_count() != 0)
#define in_hardirq()        (hardirq_count() != 0)

/* Display preempt_count state */
void print_preempt_count(const char *context)
{
    unsigned long pc = preempt_count();
    unsigned long preempt = (pc & PREEMPT_MASK) >> PREEMPT_SHIFT;
    unsigned long softirq = (pc & SOFTIRQ_MASK) >> SOFTIRQ_SHIFT;
    unsigned long hardirq = (pc & HARDIRQ_MASK) >> HARDIRQ_SHIFT;
    unsigned long nmi = (pc & NMI_MASK) >> NMI_SHIFT;
    
    printf("[%s] preempt_count=0x%08lx: preempt=%lu, softirq=%lu, hardirq=%lu, nmi=%lu\n",
           context, pc, preempt, softirq, hardirq, nmi);
    
    if (in_interrupt()) {
        printf("  → in_interrupt()=true");
        if (in_hardirq()) printf(" [HARDIRQ]");
        if (in_softirq()) printf(" [SOFTIRQ]");
        printf("\n");
    } else {
        printf("  → in_interrupt()=false [PROCESS CONTEXT]\n");
    }
}

/* Simulate do_softirq behavior */
void do_softirq(void)
{
    printf("\n=== do_softirq() ===\n");
    print_preempt_count("Entry");
    
    if (in_interrupt()) {
        printf("Already in interrupt context - returning\n");
        return;
    }
    
    printf("Processing softirqs...\n");
    
    /* Simulate entering softirq context */
    add_preempt_count(SOFTIRQ_OFFSET);
    print_preempt_count("During softirq");
    
    usleep(10000);  /* Simulate softirq processing */
    
    /* Exit softirq context */
    sub_preempt_count(SOFTIRQ_OFFSET);
    print_preempt_count("Exit");
}

/* Demonstrate preemption disabling */
void demo_preempt_disable(void)
{
    printf("\n=== Preemption Disable Demo ===\n");
    print_preempt_count("Initial");
    
    preempt_disable();
    print_preempt_count("After preempt_disable()");
    
    /* Nested disable */
    preempt_disable();
    print_preempt_count("After nested preempt_disable()");
    
    /* Nested enable */
    preempt_enable();
    print_preempt_count("After first preempt_enable()");
    
    preempt_enable();
    print_preempt_count("After second preempt_enable()");
}

/* Demonstrate bottom-half disabling */
void demo_bh_disable(void)
{
    printf("\n=== Bottom-Half Disable Demo ===\n");
    print_preempt_count("Initial");
    
    local_bh_disable();
    print_preempt_count("After local_bh_disable()");
    
    /* Now in_interrupt() should return true */
    if (in_interrupt()) {
        printf("✓ in_interrupt() correctly returns true\n");
    }
    
    local_bh_enable();
    print_preempt_count("After local_bh_enable()");
}

/* Simulate interrupt handler */
void simulated_irq_handler(void)
{
    printf("\n=== Simulated IRQ Handler ===\n");
    
    /* Enter hardirq context */
    add_preempt_count(HARDIRQ_OFFSET);
    print_preempt_count("IRQ handler entry");
    
    printf("Processing interrupt...\n");
    usleep(5000);
    
    /* Try to run softirqs (should fail) */
    do_softirq();
    
    /* Exit hardirq context */
    sub_preempt_count(HARDIRQ_OFFSET);
    print_preempt_count("IRQ handler exit");
}

/* Demonstrate combined preempt + bh disable */
void demo_combined_disable(void)
{
    printf("\n=== Combined Disable Demo ===\n");
    print_preempt_count("Initial");
    
    preempt_disable();
    print_preempt_count("After preempt_disable()");
    
    local_bh_disable();
    print_preempt_count("After local_bh_disable()");
    
    local_bh_enable();
    print_preempt_count("After local_bh_enable()");
    
    preempt_enable();
    print_preempt_count("After preempt_enable()");
}

int main(void)
{
    printf("=== Preemption Counter Demo ===\n");
    printf("Demonstrating kernel preempt_count mechanism\n\n");
    
    printf("Bit Layout:\n");
    printf("  Bits 0-7:   Preemption count (PREEMPT_OFFSET=0x%02lx)\n", PREEMPT_OFFSET);
    printf("  Bits 8-15:  Softirq count (SOFTIRQ_OFFSET=0x%02lx)\n", SOFTIRQ_OFFSET);
    printf("  Bits 16-19: Hardirq count (HARDIRQ_OFFSET=0x%02lx)\n", HARDIRQ_OFFSET);
    printf("  Bit  20:    NMI count (NMI_OFFSET=0x%02lx)\n", NMI_OFFSET);
    
    /* Run demos */
    demo_preempt_disable();
    demo_bh_disable();
    simulated_irq_handler();
    demo_combined_disable();
    
    /* Final check - should be back to zero */
    printf("\n=== Final State ===\n");
    print_preempt_count("Final");
    
    if (preempt_count() == 0) {
        printf("\n✓ SUCCESS: preempt_count restored to 0\n");
        return 0;
    } else {
        printf("\n⚠️  ERROR: preempt_count not restored (leak detected)\n");
        return 1;
    }
}
