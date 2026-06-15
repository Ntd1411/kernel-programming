# Proc Interface - Proc Filesystem Module

Module kernel tương tác với proc filesystem để hiển thị và quản lý thông tin hệ thống.

## Tổng Quan

Proc filesystem (`/proc`) là một filesystem ảo trong Linux cho phép:
- Hiển thị thông tin kernel và process
- Giao tiếp giữa user space và kernel space
- Cấu hình runtime parameters
- Debug và monitoring

## Cấu Trúc Module

### 1. proc_basic.c - Proc Entry Cơ Bản

Module tạo proc entry chỉ đọc hiển thị thông tin CPU:

**Tính năng:**
- Tạo `/proc/myproc`
- Hiển thị thông tin CPU hiện tại
- Read-only interface

**Test:**
```bash
sudo insmod proc_basic.ko
cat /proc/myproc
sudo rmmod proc_basic
```

### 2. proc_seq.c - Sequential File Operations

Module sử dụng seq_file API để hiển thị danh sách dữ liệu lớn:

**Tính năng:**
- Tạo `/proc/cpu_info`
- Hiển thị thông tin tất cả CPU
- Sử dụng iterator pattern
- Xử lý dữ liệu lớn hiệu quả

**Test:**
```bash
sudo insmod proc_seq.ko
cat /proc/cpu_info
sudo rmmod proc_seq
```

### 3. proc_rw.c - Read/Write Proc Entry

Module cho phép đọc và ghi dữ liệu:

**Tính năng:**
- Tạo `/proc/mymessage`
- Cho phép đọc và ghi
- Thread-safe với mutex
- Thống kê số lần đọc/ghi

**Test:**
```bash
sudo insmod proc_rw.ko

# Đọc message mặc định
cat /proc/mymessage

# Ghi message mới
echo "Hello from user space!" | sudo tee /proc/mymessage

# Đọc lại message
cat /proc/mymessage

# Kiểm tra kernel log
dmesg | tail

sudo rmmod proc_rw
```

## Compile và Test

### Build tất cả modules

```bash
cd 03-kernel-module/proc-interface
make
```

### Load modules

```bash
make install
```

### Test tất cả modules

```bash
make test
```

### Unload modules

```bash
make uninstall
```

### Clean

```bash
make clean
```

## Kiến Thức Cần Nắm

### Proc Filesystem API

1. **Tạo proc entry:**
```c
struct proc_dir_entry *proc_create(
    const char *name,
    umode_t mode,
    struct proc_dir_entry *parent,
    const struct proc_ops *proc_ops
);
```

2. **Xóa proc entry:**
```c
void proc_remove(struct proc_dir_entry *entry);
```

3. **Proc operations:**
```c
struct proc_ops {
    ssize_t (*proc_read)(struct file *, char __user *, size_t, loff_t *);
    ssize_t (*proc_write)(struct file *, const char __user *, size_t, loff_t *);
    int (*proc_open)(struct inode *, struct file *);
    int (*proc_release)(struct inode *, struct file *);
    loff_t (*proc_lseek)(struct file *, loff_t, int);
};
```

### Sequential File Interface

1. **Seq operations:**
```c
struct seq_operations {
    void *(*start)(struct seq_file *s, loff_t *pos);
    void *(*next)(struct seq_file *s, void *v, loff_t *pos);
    void (*stop)(struct seq_file *s, void *v);
    int (*show)(struct seq_file *s, void *v);
};
```

2. **Sử dụng seq_printf:**
```c
void seq_printf(struct seq_file *s, const char *fmt, ...);
```

### Copy Data User-Kernel Space

```c
// Copy từ user space sang kernel space
unsigned long copy_from_user(void *to, const void __user *from, unsigned long n);

// Copy từ kernel space sang user space  
unsigned long copy_to_user(void __user *to, const void *from, unsigned long n);
```

## Lưu Ý Quan Trọng

### Permissions

- `0444` - Read-only (r--r--r--)
- `0666` - Read-write (rw-rw-rw-)
- `0644` - Read-write owner, read others (rw-r--r--)

### Thread Safety

- Sử dụng `mutex` để bảo vệ shared data
- Tránh race condition khi có nhiều process truy cập

### Memory Management

- Giới hạn kích thước buffer
- Kiểm tra kết quả kmalloc
- Nhớ kfree khi cleanup

### Error Handling

- Return `-EFAULT` khi copy_to_user/copy_from_user fail
- Return `-ENOMEM` khi allocation fail
- Return `-EBUSY` khi device busy

## Debug

### Kiểm tra proc entries

```bash
# Liệt kê tất cả proc entries
ls -la /proc/

# Xem quyền truy cập
ls -l /proc/myproc /proc/cpu_info /proc/mymessage
```

### Kiểm tra kernel log

```bash
# Xem log realtime
sudo dmesg -w

# Xem log mới nhất
sudo dmesg | tail -50

# Lọc theo module
sudo dmesg | grep -i proc
```

### Kiểm tra modules đã load

```bash
lsmod | grep proc
```

## Ứng Dụng Thực Tế

1. **Monitoring:**
   - Hiển thị thông tin runtime
   - Thống kê hệ thống
   - Performance metrics

2. **Configuration:**
   - Thay đổi parameters runtime
   - Bật/tắt tính năng
   - Tuning parameters

3. **Debugging:**
   - Xem trạng thái internal
   - Dump debug information
   - Trace execution

## Bài Tập Mở Rộng

1. Tạo proc entry hiển thị memory info
2. Tạo proc entry cho phép enable/disable debug mode
3. Tạo proc entry hiển thị process list
4. Tạo proc directory với nhiều entries con
5. Implement proc entry với binary data

## Tham Khảo

- [Linux Kernel Documentation - procfs](https://www.kernel.org/doc/html/latest/filesystems/proc.html)
- [seq_file howto](https://www.kernel.org/doc/html/latest/filesystems/seq_file.html)
- The Linux Programming Interface (Chapter 12)
- Linux Device Drivers (Chapter 4)
