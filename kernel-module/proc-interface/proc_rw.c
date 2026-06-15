/*
 * proc_rw.c - Read/Write proc entry
 * 
 * Tạo proc entry cho phép đọc và ghi dữ liệu
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/smp.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kernel Linux Learning");
MODULE_DESCRIPTION("Proc read/write interface");
MODULE_VERSION("1.0");

#define PROC_NAME "mymessage"
#define MAX_SIZE 4096

static struct proc_dir_entry *proc_entry;
static char *message_buffer;
static size_t message_len = 0;
static DEFINE_MUTEX(message_mutex);
static unsigned long write_count = 0;
static unsigned long read_count = 0;

/* Ham doc du lieu tu proc entry */
static ssize_t proc_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos) {
    size_t len;
    char header[128];
    int header_len;
    
    mutex_lock(&message_mutex);
    
    if (*ppos > 0) {
        mutex_unlock(&message_mutex);
        return 0;
    }
    
    header_len = snprintf(header, sizeof(header),
                          "=== Message Buffer (CPU %d) ===\n"
                          "Read count: %lu\n"
                          "Write count: %lu\n"
                          "Message length: %zu\n"
                          "---\n",
                          smp_processor_id(), read_count, write_count, message_len);
    
    len = header_len + message_len;
    
    if (count < len)
        len = count;
    
    if (copy_to_user(ubuf, header, header_len)) {
        mutex_unlock(&message_mutex);
        return -EFAULT;
    }
    
    if (message_len > 0 && len > header_len) {
        size_t msg_bytes = len - header_len;
        if (copy_to_user(ubuf + header_len, message_buffer, msg_bytes)) {
            mutex_unlock(&message_mutex);
            return -EFAULT;
        }
    }
    
    *ppos = len;
    read_count++;
    
    printk(KERN_INFO "ProcRW: Doc %zu bytes tu /proc/%s tren CPU %d\n",
           len, PROC_NAME, smp_processor_id());
    
    mutex_unlock(&message_mutex);
    return len;
}

/* Ham ghi du lieu vao proc entry */
static ssize_t proc_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos) {
    size_t len;
    
    mutex_lock(&message_mutex);
    
    len = count;
    if (len > MAX_SIZE - 1)
        len = MAX_SIZE - 1;
    
    if (copy_from_user(message_buffer, ubuf, len)) {
        mutex_unlock(&message_mutex);
        return -EFAULT;
    }
    
    message_buffer[len] = '\0';
    message_len = len;
    write_count++;
    
    printk(KERN_INFO "ProcRW: Ghi %zu bytes vao /proc/%s tren CPU %d\n",
           len, PROC_NAME, smp_processor_id());
    printk(KERN_INFO "ProcRW: Noi dung: %s\n", message_buffer);
    
    mutex_unlock(&message_mutex);
    return len;
}

static const struct proc_ops proc_fops = {
    .proc_read  = proc_read,
    .proc_write = proc_write,
};

/* Ham khoi tao module */
static int __init proc_rw_init(void) {
    message_buffer = kmalloc(MAX_SIZE, GFP_KERNEL);
    if (!message_buffer) {
        printk(KERN_ALERT "ProcRW: Khong the cap phat bo nho\n");
        return -ENOMEM;
    }
    
    strcpy(message_buffer, "Hello! Hay ghi message cua ban vao day.\n");
    message_len = strlen(message_buffer);
    
    proc_entry = proc_create(PROC_NAME, 0666, NULL, &proc_fops);
    if (!proc_entry) {
        kfree(message_buffer);
        printk(KERN_ALERT "ProcRW: Khong the tao /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }
    
    printk(KERN_INFO "ProcRW: Tao /proc/%s thanh cong\n", PROC_NAME);
    printk(KERN_INFO "ProcRW: Doc: cat /proc/%s\n", PROC_NAME);
    printk(KERN_INFO "ProcRW: Ghi: echo 'message' > /proc/%s\n", PROC_NAME);
    return 0;
}

/* Ham cleanup module */
static void __exit proc_rw_exit(void) {
    proc_remove(proc_entry);
    kfree(message_buffer);
    printk(KERN_INFO "ProcRW: Xoa /proc/%s thanh cong\n", PROC_NAME);
    printk(KERN_INFO "ProcRW: Thong ke: %lu doc, %lu ghi\n", read_count, write_count);
}

module_init(proc_rw_init);
module_exit(proc_rw_exit);
