/*
 * proc_basic.c - Proc entry cơ bản
 * 
 * Tạo proc entry cho phép đọc thông tin từ /proc/myproc
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/smp.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kernel Linux Learning");
MODULE_DESCRIPTION("Proc filesystem interface co ban");
MODULE_VERSION("1.0");

#define PROC_NAME "myproc"

static struct proc_dir_entry *proc_entry;

/* Ham doc du lieu tu proc entry */
static ssize_t proc_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos) {
    char buf[256];
    int len;
    
    if (*ppos > 0)
        return 0;
    
    len = snprintf(buf, sizeof(buf),
                   "Hello tu proc filesystem!\n"
                   "CPU hien tai: %d\n"
                   "Tong so CPU: %d\n"
                   "Tong so CPU online: %d\n",
                   smp_processor_id(),
                   num_possible_cpus(),
                   num_online_cpus());
    
    if (count < len)
        len = count;
    
    if (copy_to_user(ubuf, buf, len))
        return -EFAULT;
    
    *ppos = len;
    printk(KERN_INFO "ProcBasic: Doc %d bytes tu /proc/%s tren CPU %d\n", 
           len, PROC_NAME, smp_processor_id());
    
    return len;
}

static const struct proc_ops proc_fops = {
    .proc_read = proc_read,
};

/* Ham khoi tao module */
static int __init proc_basic_init(void) {
    proc_entry = proc_create(PROC_NAME, 0444, NULL, &proc_fops);
    
    if (!proc_entry) {
        printk(KERN_ALERT "ProcBasic: Khong the tao /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }
    
    printk(KERN_INFO "ProcBasic: Tao /proc/%s thanh cong tren CPU %d\n", 
           PROC_NAME, smp_processor_id());
    return 0;
}

/* Ham cleanup module */
static void __exit proc_basic_exit(void) {
    proc_remove(proc_entry);
    printk(KERN_INFO "ProcBasic: Xoa /proc/%s thanh cong\n", PROC_NAME);
}

module_init(proc_basic_init);
module_exit(proc_basic_exit);
