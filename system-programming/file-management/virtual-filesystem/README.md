# Virtual Filesystem Kernel Module

## Tổng quan

Đây là ví dụ về cách tạo một filesystem đơn giản sử dụng kernel module. Module này tạo ra một filesystem ảo có thể được mount và chứa các file ảo.

## Cấu trúc

- `vfs_module.c` - Kernel module chính implement VFS
- `Makefile` - Build kernel module
- `test_vfs.sh` - Script để test module
- `CHEATSHEET.md` - Các lệnh và APIs quan trọng

## Tính năng

1. Tạo filesystem type mới có tên "simplefs"
2. Cho phép mount/umount
3. Tạo các file và thư mục ảo
4. Hỗ trợ đọc nội dung file
5. Hiển thị trong /proc/filesystems

## Cách sử dụng

### Build module

```bash
make
```

### Load module

```bash
sudo insmod vfs_module.ko
```

### Kiểm tra module đã load

```bash
lsmod | grep vfs_module
dmesg | tail
cat /proc/filesystems | grep simplefs
```

### Tạo mount point và mount

```bash
sudo mkdir -p /mnt/simplefs
sudo mount -t simplefs none /mnt/simplefs
```

### Xem nội dung

```bash
ls -la /mnt/simplefs
cat /mnt/simplefs/hello
cat /mnt/simplefs/info
```

### Umount và unload

```bash
sudo umount /mnt/simplefs
sudo rmmod vfs_module
dmesg | tail
```

## Yêu cầu

- Linux kernel headers
- GCC compiler
- Root privileges

## Cài đặt kernel headers

### Ubuntu/Debian
```bash
sudo apt-get install linux-headers-$(uname -r)
```

### Fedora/RHEL
```bash
sudo dnf install kernel-devel kernel-headers
```

### Arch Linux
```bash
sudo pacman -S linux-headers
```

## Notes

- Module này chỉ để học tập, không nên dùng trong production
- Filesystem là read-only
- Dữ liệu chỉ tồn tại trong memory
- Module cần quyền root để load/unload

## Scripts tiện ích

### demo_vfs.sh
Script demo nhanh toàn bộ chức năng:
```bash
sudo ./demo_vfs.sh
```

### test_vfs.sh
Script test chi tiết hơn với nhiều test cases:
```bash
sudo ./test_vfs.sh
```

### cleanup_vfs.sh
Script dọn dẹp khi gặp vấn đề (module bị treo, không umount được):
```bash
sudo ./cleanup_vfs.sh
```

## Troubleshooting

### Lỗi: Module is in use

```
rmmod: ERROR: Module vfs_module is in use
```

**Nguyên nhân:** Filesystem vẫn đang được mount hoặc có process đang sử dụng.

**Giải pháp:**

1. Kiểm tra mount points:
```bash
mount | grep simplefs
```

2. Umount trước:
```bash
sudo umount /mnt/simplefs
# Nếu không được, thử force:
sudo umount -f /mnt/simplefs
# Hoặc lazy umount:
sudo umount -l /mnt/simplefs
```

3. Kiểm tra processes đang sử dụng:
```bash
sudo lsof /mnt/simplefs
# Hoặc:
sudo fuser -m /mnt/simplefs
```

4. Kill processes nếu cần:
```bash
sudo fuser -km /mnt/simplefs
```

5. Thử unload lại:
```bash
sudo rmmod vfs_module
```

6. Nếu vẫn không được, chạy cleanup script:
```bash
sudo ./cleanup_vfs.sh
```

### Lỗi: Module already loaded

```
insmod: ERROR: could not insert module vfs_module.ko: File exists
```

**Giải pháp:**
```bash
# Unload module cũ trước
sudo rmmod vfs_module
# Load lại
sudo insmod vfs_module.ko
```

### Lỗi: Operation not permitted

```
mount: /mnt/simplefs: permission denied
```

**Giải pháp:** Chạy với sudo:
```bash
sudo mount -t simplefs none /mnt/simplefs
```

### Lỗi: No such device

```
mount: /mnt/simplefs: mount(2) system call failed: No such device
```

**Nguyên nhân:** Module chưa được load hoặc filesystem type chưa được đăng ký.

**Giải pháp:**
```bash
# Kiểm tra module
lsmod | grep vfs_module

# Nếu chưa có, load module
sudo insmod vfs_module.ko

# Kiểm tra filesystem type
cat /proc/filesystems | grep simplefs
```

### Lỗi: Build failed với kernel 6.8+

**Nguyên nhân:** API kernel đã thay đổi trong phiên bản mới.

**Giải pháp:** Code đã được cập nhật để hỗ trợ kernel 6.8+:
- Sử dụng `&nop_mnt_idmap` thay vì `&init_user_ns`
- Sử dụng `simple_inode_init_ts()` cho timestamps
- Đã thêm headers cần thiết

Nếu vẫn gặp lỗi, kiểm tra:
```bash
# Kernel version
uname -r

# Kernel headers
ls /lib/modules/$(uname -r)/build/
```

### Module bị treo không thể unload

**Giải pháp cấp cứu:**

1. Chạy cleanup script:
```bash
sudo ./cleanup_vfs.sh
```

2. Nếu vẫn không được, reboot:
```bash
sudo reboot
```

3. Sau khi reboot, kiểm tra:
```bash
lsmod | grep vfs_module
# Không nên có output
```

### Debug kernel messages

Xem kernel log để debug:
```bash
# Xem tất cả messages
dmesg | tail -50

# Filter theo simplefs
dmesg | grep simplefs

# Follow real-time
sudo dmesg -w
```

cd kernel-programming/system-programming/file-management/virtual-filesystem
