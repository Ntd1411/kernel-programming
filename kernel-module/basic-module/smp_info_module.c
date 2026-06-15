/*
 * smp_info_module.c - Module hiển thị thông tin SMP
 * 
 * Module chuyên dụng để hiển thị thông tin về hệ thống đa xử lý
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/smp.h>
#include <linux/cpumask.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kernel Linux Learning");
MODULE_DESCRIPTION("Module hien thi thong tin SMP");
MODULE_VERSION("1.0");

static void print_cpu_info(void *info) {
    int cpu = smp_processor_id();
    printk(KERN_INFO "  CPU %d: Online va hoat dong\n", cpu);
}

static int __init smp_info_init(void) {
    int cpu;
    
    printk(KERN_INFO "=== THONG TIN HE THONG SMP ===\n");
    
    printk(KERN_INFO "\n1. Tong quan CPU:\n");
    printk(KERN_INFO "  - So CPU co the: %d\n", num_possible_cpus());
    printk(KERN_INFO "  - So CPU online: %d\n", num_online_cpus());
    printk(KERN_INFO "  - So CPU hien dien: %d\n", num_present_cpus());
    printk(KERN_INFO "  - So CPU hoat dong: %d\n", num_active_cpus());
    
    printk(KERN_INFO "\n2. CPU hien tai:\n");
    printk(KERN_INFO "  - Module dang chay tren CPU: %d\n", smp_processor_id());
    
    printk(KERN_INFO "\n3. Danh sach CPU online:\n");
    for_each_online_cpu(cpu) {
        printk(KERN_INFO "  - CPU %d: Online\n", cpu);
    }
    
    printk(KERN_INFO "\n4. Danh sach CPU possible:\n");
    for_each_possible_cpu(cpu) {
        printk(KERN_INFO "  - CPU %d: Possible\n", cpu);
    }
    
    printk(KERN_INFO "\n5. Test chay ham tren tat ca CPU:\n");
    on_each_cpu(print_cpu_info, NULL, 1);
    
    printk(KERN_INFO "\n=== KET THUC THONG TIN SMP ===\n");
    
    return 0;
}

static void __exit smp_info_exit(void) {
    printk(KERN_INFO "SMP Info Module: Go bo khoi he thong\n");
}

module_init(smp_info_init);
module_exit(smp_info_exit);
