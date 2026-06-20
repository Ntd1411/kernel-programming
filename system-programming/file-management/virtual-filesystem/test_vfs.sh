#!/bin/bash
#
# test_vfs.sh - Test Virtual Filesystem Module
# 
# Script kiểm tra module VFS đơn giản
# 
# Chạy: sudo ./test_vfs.sh

set -e

MODULE="vfs_module"
MOUNT_PT="/mnt/vfs_demo"

# Kiểm tra quyền root
if [ "$EUID" -ne 0 ]; then
    echo "Loi: Script nay can quyen root/sudo"
    exit 1
fi

echo "=== Dọn dẹp trạng thái trước đó ==="
umount "$MOUNT_PT" 2>/dev/null || true
rmmod "$MODULE" 2>/dev/null || true
sleep 1

echo ""
echo "=== Biên dịch module ==="
make -f Makefile clean
make -f Makefile

echo ""
echo "=== Load module vào kernel ==="
insmod "${MODULE}.ko"
sleep 1

echo ""
echo "Kiểm tra dmesg (khởi tạo module):"
dmesg | tail -15

echo ""
echo "=== Tạo mount point ==="
mkdir -p "$MOUNT_PT"
echo "Mount point: $MOUNT_PT"

echo ""
echo "=== Mount filesystem ==="
mount -t simplefs none "$MOUNT_PT"
sleep 1

echo ""
echo "Kiểm tra dmesg (mount):"
dmesg | tail -10

echo ""
echo "=== Liệt kê nội dung thư mục ==="
ls -la "$MOUNT_PT"

echo ""
echo "=== Đọc nội dung file hello ==="
cat "$MOUNT_PT/hello"

echo ""
echo "=== Đọc nội dung file info ==="
cat "$MOUNT_PT/info"

echo ""
echo "=== Umount filesystem ==="
umount "$MOUNT_PT"
sleep 1

echo ""
echo "Kiểm tra dmesg (umount):"
dmesg | tail -10

echo ""
echo "=== Gỡ bỏ module khỏi kernel ==="
rmmod "$MODULE"
sleep 1

echo ""
echo "Kiểm tra dmesg (thoát module):"
dmesg | tail -10

echo ""
echo "================================================"
echo "Test hoàn tất thành công"
echo "================================================"