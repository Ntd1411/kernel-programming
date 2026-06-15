# Basic Module - Module Nhân Cơ Bản

Tập hợp các kernel module cơ bản minh họa cách viết và tích hợp module vào Linux kernel.

## Các Module

### 1. hello_module.c
Module đơn giản nhất, in thông báo khi load và unload.

**Tính năng:**
- In thông báo Hello khi load
- Hiển thị CPU đang chạy module
- Hiển thị số CPU trong hệ thống

**Cách sử dụng:**
```bash
make
sudo insmod hello_module.ko
dmesg | tail
sudo rmmod hello_module
```

### 2. param_module.c
Module cho phép truyền tham số khi load.

**Tham số:**
- `name` (string): Tên để chào (mặc định: "World")
- `count` (int): Số lần in thông báo (mặc định: 1)
- `verbose` (bool): Chế độ chi tiết (mặc định: false)

**Cách sử dụng:**
```bash
make
sudo insmod param_module.ko name="Linux" count=5 verbose=1
dmesg | tail
sudo rmmod param_module
```

**Xem và thay đổi tham số runtime:**
```bash
cat /sys/module/param_module/parameters/name
echo "NewName" | sudo tee /sys/module/param_module/parameters/name
```

### 3. smp_info_module.c
Module chuyên dụng hiển thị thông tin về hệ thống SMP (Symmetric Multi-Processing).

**Thông tin hiển thị:**
- Số CPU possible, online, present, active
- CPU đang chạy module
- Danh sách tất cả CPU
- Test chạy hàm trên tất cả CPU

**Cách sử dụng:**
```bash
make
sudo insmod smp_info_module.ko
dmesg | tail -n 30
sudo rmmod smp_info_module
```

## Biên Dịch

```bash
# Biên dịch tất cả module
make

# Xóa file biên dịch
make clean

# Cài đặt module vào hệ thống
sudo make install
```

## Load và Unload Module

### Cách 1: Sử dụng insmod/rmmod
```bash
# Load module
sudo insmod hello_module.ko

# Unload module
sudo rmmod hello_module
```

### Cách 2: Sử dụng modprobe (sau khi install)
```bash
# Load module
sudo modprobe hello_module

# Unload module
sudo modprobe -r hello_module
```

## Kiểm Tra Module

```bash
# Liệt kê module đã load
lsmod | grep module

# Xem thông tin chi tiết module
modinfo hello_module.ko

# Xem kernel log
dmesg | tail
sudo journalctl -k -n 20

# Xem thông tin module đang chạy
cat /proc/modules | grep module
```

## Quy Trình Phát Triển Module

### 1. Viết Code
- Tạo file .c với các hàm init và exit
- Khai báo MODULE_LICENSE, MODULE_AUTHOR, MODULE_DESCRIPTION
- Implement các chức năng cần thiết

### 2. Tạo Makefile
```makefile
obj-m += ten_module.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
```

### 3. Biên Dịch
```bash
make
```

### 4. Test
```bash
sudo insmod ten_module.ko
dmesg | tail
sudo rmmod ten_module
```

### 5. Debug
```bash
# Xem log chi tiết
dmesg -w

# Kiểm tra lỗi
journalctl -k -f

# Kiểm tra module info
modinfo ten_module.ko
```

## Cấu Trúc Module Cơ Bản

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ten tac gia");
MODULE_DESCRIPTION("Mo ta module");
MODULE_VERSION("1.0");

static int __init ten_module_init(void) {
    printk(KERN_INFO "Module: Khoi tao\n");
    return 0;
}

static void __exit ten_module_exit(void) {
    printk(KERN_INFO "Module: Ket thuc\n");
}

module_init(ten_module_init);
module_exit(ten_module_exit);
```

## Lưu Ý Quan Trọng

### Về SMP (Symmetric Multi-Processing)
- Module có thể chạy trên bất kỳ CPU nào
- Cần xử lý đồng bộ hóa khi truy cập dữ liệu chung
- Sử dụng spinlock, mutex, atomic operations

### Về Memory
- Không dùng malloc/free, dùng kmalloc/kfree
- Kernel space memory khác user space
- Cẩn thận với stack size (thường 8KB)

### Về Logging
- Dùng printk thay vì printf
- Các mức độ: KERN_EMERG, KERN_ALERT, KERN_CRIT, KERN_ERR, KERN_WARNING, KERN_NOTICE, KERN_INFO, KERN_DEBUG

### Về License
- GPL: Bắt buộc cho module tương tác sâu với kernel
- Không GPL: Module có thể bị kernel từ chối load

## Tích Hợp Vào Hệ Thống

### Auto-load khi boot
```bash
# Copy module vào thư mục hệ thống
sudo cp ten_module.ko /lib/modules/$(uname -r)/kernel/drivers/

# Cập nhật module dependencies
sudo depmod -a

# Thêm vào /etc/modules
echo "ten_module" | sudo tee -a /etc/modules
```

### Cấu hình parameters
```bash
# Tạo file cấu hình
sudo nano /etc/modprobe.d/ten_module.conf

# Nội dung:
options ten_module param1=value1 param2=value2
```

## Troubleshooting

### Module không load được
```bash
# Kiểm tra lỗi chi tiết
sudo insmod module.ko
dmesg | tail

# Kiểm tra kernel version
uname -r
modinfo module.ko | grep vermagic
```

### Module bị stuck
```bash
# Kiểm tra reference count
lsmod | grep module_name

# Force unload (không khuyến khích)
sudo rmmod -f module_name
```

### Lỗi biên dịch
```bash
# Cài đặt kernel headers
sudo apt install linux-headers-$(uname -r)

# Kiểm tra đường dẫn kernel source
ls -la /lib/modules/$(uname -r)/build
```

## Tài Liệu Tham Khảo

- Linux Kernel Module Programming Guide
- Linux Device Drivers (3rd Edition)
- Kernel documentation: /usr/src/linux/Documentation
