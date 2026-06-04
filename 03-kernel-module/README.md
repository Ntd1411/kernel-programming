# Phần 3: Lập Trình Module Nhân Linux

Xây dựng Loadable Kernel Module (LKM) và tích hợp vào Linux kernel.

## Mục Tiêu

- Hiểu kiến trúc Linux kernel
- Viết và compile kernel module
- Tạo character device driver
- Giao tiếp user space - kernel space
- Sử dụng proc filesystem

## Cấu Trúc

### 1. Basic Module (basic-module/)

Module cơ bản:
- `hello_module.c` - Module Hello World
- `param_module.c` - Module với parameters
- `info_module.c` - Module hiển thị thông tin hệ thống
- `Makefile` - Build kernel module

### 2. Character Device (char-device/)

Character device driver:
- `chardev.c` - Character device cơ bản
- `chardev_advanced.c` - Character device với ioctl
- `device_test.c` - User space test program

### 3. Proc Interface (proc-interface/)

Proc filesystem:
- `proc_basic.c` - Proc entry cơ bản
- `proc_seq.c` - Sequential file operations
- `proc_rw.c` - Read/Write proc entry

## Yêu Cầu

```bash
# Cài đặt kernel headers
sudo apt install linux-headers-$(uname -r)

# Kiểm tra kernel version
uname -r
```

## Cấu Trúc Module Cơ Bản

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple kernel module");
MODULE_VERSION("1.0");

static int __init hello_init(void) {
    printk(KERN_INFO "Hello, Kernel!\n");
    return 0;
}

static void __exit hello_exit(void) {
    printk(KERN_INFO "Goodbye, Kernel!\n");
}

module_init(hello_init);
module_exit(hello_exit);
```

## Makefile Mẫu

```makefile
obj-m += hello_module.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
```

## Compile và Load Module

```bash
# Compile module
make

# Load module
sudo insmod hello_module.ko

# Kiểm tra module đã load
lsmod | grep hello_module

# Xem kernel log
dmesg | tail
sudo journalctl -k | tail

# Xem thông tin module
modinfo hello_module.ko

# Unload module
sudo rmmod hello_module

# Load với modprobe (tự động xử lý dependencies)
sudo modprobe hello_module
```

## Character Device Driver

### Cấu trúc cơ bản:

```c
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

static int major_number;
static struct cdev my_cdev;

static int device_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "Device opened\n");
    return 0;
}

static int device_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "Device closed\n");
    return 0;
}

static ssize_t device_read(struct file *file, char __user *buf,
                           size_t len, loff_t *offset) {
    // Read implementation
    return 0;
}

static ssize_t device_write(struct file *file, const char __user *buf,
                            size_t len, loff_t *offset) {
    // Write implementation
    return len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .read = device_read,
    .write = device_write,
};

static int __init chardev_init(void) {
    major_number = register_chrdev(0, "mydevice", &fops);
    return 0;
}

static void __exit chardev_exit(void) {
    unregister_chrdev(major_number, "mydevice");
}
```

### Tạo device file:

```bash
# Tìm major number
cat /proc/devices | grep mydevice

# Tạo device node
sudo mknod /dev/mydevice c <major_number> 0
sudo chmod 666 /dev/mydevice

# Test device
echo "Hello" > /dev/mydevice
cat /dev/mydevice
```

## Proc Filesystem

```c
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

static struct proc_dir_entry *proc_entry;

static int proc_show(struct seq_file *m, void *v) {
    seq_printf(m, "Hello from kernel!\n");
    return 0;
}

static int proc_open(struct inode *inode, struct file *file) {
    return single_open(file, proc_show, NULL);
}

static const struct proc_ops proc_fops = {
    .proc_open = proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static int __init proc_init(void) {
    proc_entry = proc_create("myproc", 0, NULL, &proc_fops);
    return 0;
}

static void __exit proc_exit(void) {
    proc_remove(proc_entry);
}
```

## Bài Tập

1. Viết module hiển thị thông tin CPU và memory
2. Tạo character device đọc/ghi data vào buffer
3. Xây dựng module đếm số lần system call được gọi
4. Tạo virtual device mô phỏng cảm biến nhiệt độ
5. Viết module hook vào network stack

## Debug Kernel Module

```bash
# Xem kernel messages
dmesg
dmesg -w  # Watch mode

# Kernel log levels
printk(KERN_EMERG "Emergency\n");
printk(KERN_ALERT "Alert\n");
printk(KERN_CRIT "Critical\n");
printk(KERN_ERR "Error\n");
printk(KERN_WARNING "Warning\n");
printk(KERN_NOTICE "Notice\n");
printk(KERN_INFO "Info\n");
printk(KERN_DEBUG "Debug\n");

# Debug với printk
#define DEBUG 1
#ifdef DEBUG
    printk(KERN_DEBUG "Debug: %s\n", message);
#endif
```

## Lưu Ý Quan Trọng

- Kernel code không có access đến standard C library
- Không được sleep trong interrupt context
- Cẩn thận với kernel panics
- Luôn test trên máy ảo trước
- Backup hệ thống trước khi test
- Module phải compatible với kernel version
- Không dùng floating point trong kernel
- Memory allocation: `kmalloc()`, `kfree()`
- Copy data giữa kernel/user space: `copy_to_user()`, `copy_from_user()`

## Tài Liệu Tham Khảo

- Linux Device Drivers (LDD3)
- Linux Kernel Development (Robert Love)
- The Linux Kernel Module Programming Guide
- Kernel source code: `/usr/src/linux-headers-$(uname -r)/`
