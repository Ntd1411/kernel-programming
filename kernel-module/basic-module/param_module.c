/*
 * param_module.c - Module với parameters
 * 
 * Minh họa cách truyền tham số cho module khi load
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kernel Linux Learning");
MODULE_DESCRIPTION("Module voi parameters");
MODULE_VERSION("1.0");

static char *name = "World";
static int count = 1;
static bool verbose = false;

module_param(name, charp, 0644);
MODULE_PARM_DESC(name, "Ten de chao mung (mac dinh: World)");

module_param(count, int, 0644);
MODULE_PARM_DESC(count, "So lan in ra thong bao (mac dinh: 1)");

module_param(verbose, bool, 0644);
MODULE_PARM_DESC(verbose, "Che do chi tiet (mac dinh: false)");

static int __init param_init(void) {
    int i;
    
    printk(KERN_INFO "Param Module: Khoi tao voi cac tham so:\n");
    printk(KERN_INFO "  - name: %s\n", name);
    printk(KERN_INFO "  - count: %d\n", count);
    printk(KERN_INFO "  - verbose: %s\n", verbose ? "true" : "false");
    
    if (verbose) {
        printk(KERN_INFO "Param Module: Thong tin CPU:\n");
        printk(KERN_INFO "  - CPU hien tai: %d\n", smp_processor_id());
        printk(KERN_INFO "  - So CPU online: %d\n", num_online_cpus());
        printk(KERN_INFO "  - So CPU co the: %d\n", num_possible_cpus());
    }
    
    for (i = 0; i < count; i++) {
        printk(KERN_INFO "Hello, %s! (lan %d/%d)\n", name, i + 1, count);
    }
    
    return 0;
}

static void __exit param_exit(void) {
    printk(KERN_INFO "Param Module: Goodbye, %s!\n", name);
}

module_init(param_init);
module_exit(param_exit);
