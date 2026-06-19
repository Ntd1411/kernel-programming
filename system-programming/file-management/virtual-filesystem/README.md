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

cd kernel-programming/system-programming/file-management/
