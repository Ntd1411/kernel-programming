# Proc Interface Examples - Cheat Sheet

## Quick Start

```bash
# Build
make

# Test nhanh
make test

# Demo tuong tac
sudo ./demo_proc.sh

# Cleanup
make clean
```

## Cac Proc Entries

| Entry | Path | Permissions | Muc dich |
|-------|------|-------------|----------|
| proc_basic | /proc/myproc | 0444 (r--r--r--) | Hien thi thong tin CPU |
| proc_seq | /proc/cpu_info | 0444 (r--r--r--) | Danh sach tat ca CPU |
| proc_rw | /proc/mymessage | 0666 (rw-rw-rw-) | Message buffer doc/ghi |

## Lenh Thu Cong

### Proc Basic

```bash
sudo insmod proc_basic.ko
cat /proc/myproc
sudo rmmod proc_basic
```

### Proc Seq

```bash
sudo insmod proc_seq.ko
cat /proc/cpu_info
sudo rmmod proc_seq
```

### Proc RW

```bash
sudo insmod proc_rw.ko

# Doc
cat /proc/mymessage

# Ghi
echo "Your message" > /proc/mymessage

# Doc lai
cat /proc/mymessage

sudo rmmod proc_rw
```

## Debug Commands

```bash
# Xem kernel log realtime
sudo dmesg -w

# Loc log theo module
sudo dmesg | grep -i proc

# Kiem tra modules dang load
lsmod | grep proc

# Xem chi tiet module
modinfo proc_basic.ko

# Xem permissions
ls -l /proc/myproc /proc/cpu_info /proc/mymessage
```

## API Reference

### Tao Proc Entry

```c
struct proc_dir_entry *proc_create(
    const char *name,        // Ten entry
    umode_t mode,            // Permissions (0444, 0666, etc.)
    struct proc_dir_entry *parent,  // NULL = /proc root
    const struct proc_ops *proc_ops
);
```

### Proc Operations

```c
struct proc_ops {
    ssize_t (*proc_read)(struct file *, char __user *, size_t, loff_t *);
    ssize_t (*proc_write)(struct file *, const char __user *, size_t, loff_t *);
    int (*proc_open)(struct inode *, struct file *);
    int (*proc_release)(struct inode *, struct file *);
    loff_t (*proc_lseek)(struct file *, loff_t, int);
};
```

### Sequential File Operations

```c
struct seq_operations {
    void *(*start)(struct seq_file *, loff_t *pos);  // Bat dau iteration
    void *(*next)(struct seq_file *, void *v, loff_t *pos);  // Phan tu tiep theo
    void (*stop)(struct seq_file *, void *v);  // Ket thuc
    int (*show)(struct seq_file *, void *v);   // Hien thi du lieu
};
```

### Copy Data

```c
// User -> Kernel
unsigned long copy_from_user(void *to, const void __user *from, unsigned long n);

// Kernel -> User
unsigned long copy_to_user(void __user *to, const void *from, unsigned long n);
```

## Common Patterns

### Read-only Proc Entry

```c
static ssize_t proc_read(struct file *file, char __user *ubuf, 
                         size_t count, loff_t *ppos) {
    char buf[256];
    int len;
    
    if (*ppos > 0)
        return 0;
    
    len = snprintf(buf, sizeof(buf), "Your data here\n");
    
    if (copy_to_user(ubuf, buf, len))
        return -EFAULT;
    
    *ppos = len;
    return len;
}
```

### Write Proc Entry

```c
static ssize_t proc_write(struct file *file, const char __user *ubuf,
                          size_t count, loff_t *ppos) {
    char buf[256];
    size_t len = count;
    
    if (len > sizeof(buf) - 1)
        len = sizeof(buf) - 1;
    
    if (copy_from_user(buf, ubuf, len))
        return -EFAULT;
    
    buf[len] = '\0';
    
    // Xu ly du lieu...
    
    return len;
}
```

### Sequential File

```c
static void *seq_start(struct seq_file *s, loff_t *pos) {
    if (*pos >= MAX_ITEMS)
        return NULL;
    return pos;
}

static void *seq_next(struct seq_file *s, void *v, loff_t *pos) {
    (*pos)++;
    if (*pos >= MAX_ITEMS)
        return NULL;
    return pos;
}

static void seq_stop(struct seq_file *s, void *v) {
    // Cleanup
}

static int seq_show(struct seq_file *s, void *v) {
    loff_t *idx = (loff_t *)v;
    seq_printf(s, "Item %lld\n", *idx);
    return 0;
}
```

## Permissions

| Mode | Octal | Symbolic | Muc dich |
|------|-------|----------|----------|
| Read-only | 0444 | r--r--r-- | Chi doc |
| Write-only | 0222 | -w--w--w- | Chi ghi |
| Read-write | 0666 | rw-rw-rw- | Doc va ghi |
| Owner RW, others R | 0644 | rw-r--r-- | Owner doc/ghi, con lai chi doc |

## Error Codes

| Code | Constant | Y nghia |
|------|----------|---------|
| -14 | -EFAULT | Loi copy_to_user/copy_from_user |
| -12 | -ENOMEM | Khong du bo nho |
| -16 | -EBUSY | Device dang duoc su dung |
| -22 | -EINVAL | Tham so khong hop le |

## Troubleshooting

### Module khong load duoc

```bash
# Kiem tra kernel log
dmesg | tail

# Kiem tra dependencies
modinfo your_module.ko

# Kiem tra kernel version
uname -r
ls /lib/modules/$(uname -r)/build
```

### Proc entry khong xuat hien

```bash
# Kiem tra proc_create return value trong code
# Kiem tra kernel log
dmesg | grep -i proc

# List tat ca proc entries
ls -la /proc/ | grep my
```

### Permission denied

```bash
# Kiem tra permissions
ls -l /proc/your_entry

# Chay voi sudo
sudo cat /proc/your_entry
sudo echo "data" > /proc/your_entry
```

### Copy failed

```c
// Luon kiem tra ket qua copy
if (copy_to_user(ubuf, buf, len)) {
    printk(KERN_ERR "copy_to_user failed\n");
    return -EFAULT;
}

if (copy_from_user(buf, ubuf, len)) {
    printk(KERN_ERR "copy_from_user failed\n");
    return -EFAULT;
}
```

## Best Practices

1. Luon kiem tra return value cua proc_create
2. Su dung mutex de bao ve shared data
3. Gioi han buffer size de tranh overflow
4. Handle *ppos dung cach (tranh doc lap lai)
5. Return dung so byte da doc/ghi
6. Cleanup proc entry trong exit function
7. Log thong tin debug voi printk
8. Test voi multiple concurrent access

## References

- [Linux Kernel Proc Documentation](https://www.kernel.org/doc/html/latest/filesystems/proc.html)
- [Seq_file Interface](https://www.kernel.org/doc/html/latest/filesystems/seq_file.html)
- Linux Device Drivers, 3rd Edition - Chapter 4
