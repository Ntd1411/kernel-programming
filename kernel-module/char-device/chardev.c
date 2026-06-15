/*
 * chardev.c - Character Device Driver cơ bản
 * 
 * Tạo một character device cho phép read/write từ user space
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
MODULE_DESCRIPTION("Character device driver co ban");
MODULE_VERSION("1.0");

#define DEVICE_NAME "mychardev"
#define CLASS_NAME "mychar"
#define BUFFER_SIZE 1024

static int major_number;
static struct class *char_class = NULL;
static struct device *char_device = NULL;
static char *device_buffer;
static int buffer_size = 0;
static DEFINE_MUTEX(char_mutex);

static int device_open_count = 0;

/* Ham xu ly khi open device */
static int dev_open(struct inode *inodep, struct file *filep) {
    if (!mutex_trylock(&char_mutex)) {
        printk(KERN_ALERT "CharDev: Device dang duoc su dung boi tien trinh khac\n");
        return -EBUSY;
    }
    
    device_open_count++;
    printk(KERN_INFO "CharDev: Device duoc mo lan thu %d tren CPU %d\n", 
           device_open_count, smp_processor_id());
    return 0;
}

/* Ham xu ly khi close device */
static int dev_release(struct inode *inodep, struct file *filep) {
    mutex_unlock(&char_mutex);
    printk(KERN_INFO "CharDev: Device duoc dong tren CPU %d\n", smp_processor_id());
    return 0;
}

/* Ham xu ly khi read tu device */
static ssize_t dev_read(struct file *filep, char __user *buffer, 
                        size_t len, loff_t *offset) {
    int bytes_to_read;
    int bytes_read;
    
    printk(KERN_INFO "CharDev: Read request %zu bytes tu offset %lld tren CPU %d\n", 
           len, *offset, smp_processor_id());
    
    if (*offset >= buffer_size) {
        return 0;
    }
    
    bytes_to_read = min(len, (size_t)(buffer_size - *offset));
    
    bytes_read = bytes_to_read - copy_to_user(buffer, device_buffer + *offset, bytes_to_read);
    
    if (bytes_read == 0) {
        printk(KERN_ERR "CharDev: Loi khi copy du lieu sang user space\n");
        return -EFAULT;
    }
    
    *offset += bytes_read;
    printk(KERN_INFO "CharDev: Da doc %d bytes\n", bytes_read);
    
    return bytes_read;
}

/* Ham xu ly khi write vao device */
static ssize_t dev_write(struct file *filep, const char __user *buffer, 
                         size_t len, loff_t *offset) {
    int bytes_to_write;
    int bytes_written;
    
    printk(KERN_INFO "CharDev: Write request %zu bytes tren CPU %d\n", 
           len, smp_processor_id());
    
    if (len > BUFFER_SIZE) {
        printk(KERN_WARNING "CharDev: Du lieu qua lon, chi ghi %d bytes\n", BUFFER_SIZE);
        len = BUFFER_SIZE;
    }
    
    bytes_to_write = len;
    bytes_written = bytes_to_write - copy_from_user(device_buffer, buffer, bytes_to_write);
    
    if (bytes_written == 0) {
        printk(KERN_ERR "CharDev: Loi khi copy du lieu tu user space\n");
        return -EFAULT;
    }
    
    buffer_size = bytes_written;
    printk(KERN_INFO "CharDev: Da ghi %d bytes\n", bytes_written);
    
    return bytes_written;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};

static int __init chardev_init(void) {
    printk(KERN_INFO "CharDev: Khoi tao module tren CPU %d\n", smp_processor_id());
    
    /* Cap phat buffer */
    device_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!device_buffer) {
        printk(KERN_ERR "CharDev: Khong the cap phat bo nho cho buffer\n");
        return -ENOMEM;
    }
    memset(device_buffer, 0, BUFFER_SIZE);
    
    /* Dang ky major number */
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ERR "CharDev: Khong the dang ky major number\n");
        kfree(device_buffer);
        return major_number;
    }
    printk(KERN_INFO "CharDev: Dang ky thanh cong voi major number %d\n", major_number);
    
    /* Tao device class */
    char_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(char_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        kfree(device_buffer);
        printk(KERN_ERR "CharDev: Khong the tao device class\n");
        return PTR_ERR(char_class);
    }
    printk(KERN_INFO "CharDev: Device class dang ky thanh cong\n");
    
    /* Tao device */
    char_device = device_create(char_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(char_device)) {
        class_destroy(char_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        kfree(device_buffer);
        printk(KERN_ERR "CharDev: Khong the tao device\n");
        return PTR_ERR(char_device);
    }
    
    printk(KERN_INFO "CharDev: Device /dev/%s da duoc tao thanh cong\n", DEVICE_NAME);
    printk(KERN_INFO "CharDev: Su dung lenh sau de test:\n");
    printk(KERN_INFO "  echo 'Hello World' > /dev/%s\n", DEVICE_NAME);
    printk(KERN_INFO "  cat /dev/%s\n", DEVICE_NAME);
    
    return 0;
}

static void __exit chardev_exit(void) {
    printk(KERN_INFO "CharDev: Go bo module tren CPU %d\n", smp_processor_id());
    
    device_destroy(char_class, MKDEV(major_number, 0));
    class_unregister(char_class);
    class_destroy(char_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    kfree(device_buffer);
    
    printk(KERN_INFO "CharDev: Device /dev/%s da duoc go bo\n", DEVICE_NAME);
}

module_init(chardev_init);
module_exit(chardev_exit);
