/*
 * chardev_ioctl.c - Character Device với IOCTL
 * 
 * Character device hỗ trợ ioctl để điều khiển device
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kernel Linux Learning");
MODULE_DESCRIPTION("Character device voi ioctl");
MODULE_VERSION("1.0");

#define DEVICE_NAME "mychardev_ioctl"
#define CLASS_NAME "mychar_ioctl"
#define BUFFER_SIZE 1024

/* Dinh nghia IOCTL commands */
#define IOCTL_MAGIC 'k'
#define IOCTL_GET_SIZE _IOR(IOCTL_MAGIC, 1, int)
#define IOCTL_CLEAR_BUFFER _IO(IOCTL_MAGIC, 2)
#define IOCTL_GET_CPU _IOR(IOCTL_MAGIC, 3, int)
#define IOCTL_GET_STATS _IOR(IOCTL_MAGIC, 4, struct device_stats)

struct device_stats {
    int read_count;
    int write_count;
    int ioctl_count;
    int current_size;
};

static int major_number;
static struct class *char_class = NULL;
static struct device *char_device = NULL;
static char *device_buffer;
static int buffer_size = 0;
static DEFINE_MUTEX(char_mutex);

static struct device_stats stats = {0, 0, 0, 0};

static int dev_open(struct inode *inodep, struct file *filep) {
    if (!mutex_trylock(&char_mutex)) {
        return -EBUSY;
    }
    printk(KERN_INFO "CharDevIoctl: Device opened tren CPU %d\n", smp_processor_id());
    return 0;
}

static int dev_release(struct inode *inodep, struct file *filep) {
    mutex_unlock(&char_mutex);
    printk(KERN_INFO "CharDevIoctl: Device closed\n");
    return 0;
}

static ssize_t dev_read(struct file *filep, char __user *buffer, 
                        size_t len, loff_t *offset) {
    int bytes_to_read;
    int bytes_read;
    
    stats.read_count++;
    
    if (*offset >= buffer_size) {
        return 0;
    }
    
    bytes_to_read = min(len, (size_t)(buffer_size - *offset));
    bytes_read = bytes_to_read - copy_to_user(buffer, device_buffer + *offset, bytes_to_read);
    
    if (bytes_read == 0) {
        return -EFAULT;
    }
    
    *offset += bytes_read;
    printk(KERN_INFO "CharDevIoctl: Read %d bytes (total reads: %d)\n", 
           bytes_read, stats.read_count);
    
    return bytes_read;
}

static ssize_t dev_write(struct file *filep, const char __user *buffer, 
                         size_t len, loff_t *offset) {
    int bytes_to_write;
    int bytes_written;
    
    stats.write_count++;
    
    if (len > BUFFER_SIZE) {
        len = BUFFER_SIZE;
    }
    
    bytes_to_write = len;
    bytes_written = bytes_to_write - copy_from_user(device_buffer, buffer, bytes_to_write);
    
    if (bytes_written == 0) {
        return -EFAULT;
    }
    
    buffer_size = bytes_written;
    stats.current_size = buffer_size;
    
    printk(KERN_INFO "CharDevIoctl: Write %d bytes (total writes: %d)\n", 
           bytes_written, stats.write_count);
    
    return bytes_written;
}

static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    int ret = 0;
    int cpu;
    
    stats.ioctl_count++;
    
    printk(KERN_INFO "CharDevIoctl: IOCTL command %u tren CPU %d\n", 
           cmd, smp_processor_id());
    
    switch (cmd) {
        case IOCTL_GET_SIZE:
            printk(KERN_INFO "CharDevIoctl: GET_SIZE command\n");
            if (copy_to_user((int __user *)arg, &buffer_size, sizeof(int))) {
                return -EFAULT;
            }
            break;
            
        case IOCTL_CLEAR_BUFFER:
            printk(KERN_INFO "CharDevIoctl: CLEAR_BUFFER command\n");
            memset(device_buffer, 0, BUFFER_SIZE);
            buffer_size = 0;
            stats.current_size = 0;
            printk(KERN_INFO "CharDevIoctl: Buffer da duoc xoa\n");
            break;
            
        case IOCTL_GET_CPU:
            cpu = smp_processor_id();
            printk(KERN_INFO "CharDevIoctl: GET_CPU command - CPU %d\n", cpu);
            if (copy_to_user((int __user *)arg, &cpu, sizeof(int))) {
                return -EFAULT;
            }
            break;
            
        case IOCTL_GET_STATS:
            printk(KERN_INFO "CharDevIoctl: GET_STATS command\n");
            stats.current_size = buffer_size;
            if (copy_to_user((struct device_stats __user *)arg, &stats, 
                           sizeof(struct device_stats))) {
                return -EFAULT;
            }
            break;
            
        default:
            printk(KERN_WARNING "CharDevIoctl: Lenh IOCTL khong hop le: %u\n", cmd);
            return -EINVAL;
    }
    
    return ret;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
    .unlocked_ioctl = dev_ioctl,
};

static int __init chardev_ioctl_init(void) {
    printk(KERN_INFO "CharDevIoctl: Khoi tao module\n");
    
    device_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!device_buffer) {
        return -ENOMEM;
    }
    memset(device_buffer, 0, BUFFER_SIZE);
    
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        kfree(device_buffer);
        return major_number;
    }
    
    char_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(char_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        kfree(device_buffer);
        return PTR_ERR(char_class);
    }
    
    char_device = device_create(char_class, NULL, MKDEV(major_number, 0), 
                                NULL, DEVICE_NAME);
    if (IS_ERR(char_device)) {
        class_destroy(char_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        kfree(device_buffer);
        return PTR_ERR(char_device);
    }
    
    printk(KERN_INFO "CharDevIoctl: Device /dev/%s da duoc tao\n", DEVICE_NAME);
    
    return 0;
}

static void __exit chardev_ioctl_exit(void) {
    device_destroy(char_class, MKDEV(major_number, 0));
    class_destroy(char_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    kfree(device_buffer);
    
    printk(KERN_INFO "CharDevIoctl: Module da duoc go bo\n");
    printk(KERN_INFO "CharDevIoctl: Thong ke cuoi cung - Reads: %d, Writes: %d, Ioctls: %d\n",
           stats.read_count, stats.write_count, stats.ioctl_count);
}

module_init(chardev_ioctl_init);
module_exit(chardev_ioctl_exit);
