/*
 * hello_module.c - Module kernel cơ bản
 * 
 * Module đơn giản in ra thông báo khi load và unload
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kernel Linux Learning");
MODULE_DESCRIPTION("Module kernel co ban - Hello World");
MODULE_VERSION("1.0");

static int __init hello_init(void) {
    printk(KERN_INFO "Hello Module: Khoi tao thanh cong\n");
    printk(KERN_INFO "Hello Module: Chay tren CPU %d\n", smp_processor_id());
    printk(KERN_INFO "Hello Module: So CPU trong he thong: %d\n", num_online_cpus());
    return 0;
}

static void __exit hello_exit(void) {
    printk(KERN_INFO "Hello Module: Gỡ bo module\n");
    printk(KERN_INFO "Hello Module: Tam biet tu CPU %d\n", smp_processor_id());
}

module_init(hello_init);
module_exit(hello_exit);
