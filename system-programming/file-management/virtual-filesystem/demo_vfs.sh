#!/bin/bash

# demo_vfs.sh - Script demo nhanh SimplFS
# Chay: sudo ./demo_vfs.sh

MOUNT_POINT="/mnt/simplefs"

echo "=========================================="
echo "SimplFS Kernel Module - Quick Demo"
echo "=========================================="
echo ""

# Kiem tra quyen root
if [ "$EUID" -ne 0 ]; then
    echo "Loi: Script nay can quyen root"
    echo "Vui long chay: sudo ./demo_vfs.sh"
    exit 1
fi

echo "1. Build kernel module..."
make clean > /dev/null 2>&1
if make > /dev/null 2>&1; then
    echo "   [OK] Build thanh cong"
else
    echo "   [FAIL] Build that bai"
    exit 1
fi
echo ""

echo "2. Load module vao kernel..."

# Kiem tra module da duoc load chua
if lsmod | grep -q "^vfs_module"; then
    echo "   [WARN] Module da duoc load truoc do"
    echo "   Dang cleanup..."
    
    # Thu umount neu co mount point
    if mount | grep -q simplefs; then
        umount -f ${MOUNT_POINT} 2>/dev/null || umount -l ${MOUNT_POINT} 2>/dev/null
        sleep 1
    fi
    
    # Thu unload module cu
    if ! rmmod vfs_module 2>/dev/null; then
        echo "   [ERROR] Khong the unload module cu"
        echo "   Module dang duoc su dung hoac can force remove"
        echo ""
        echo "   Thu cac lenh sau:"
        echo "   1. umount -l ${MOUNT_POINT}"
        echo "   2. rmmod -f vfs_module"
        echo "   3. Hoac reboot: sudo reboot"
        exit 1
    fi
    sleep 1
fi

if insmod vfs_module.ko 2>/dev/null; then
    echo "   [OK] Module da duoc load"
else
    echo "   [FAIL] Khong the load module"
    dmesg | tail -20
    exit 1
fi
echo ""

echo "3. Kiem tra filesystem type..."
if cat /proc/filesystems | grep -q simplefs; then
    echo "   [OK] Filesystem 'simplefs' da duoc dang ky"
else
    echo "   [FAIL] Filesystem chua duoc dang ky"
    rmmod vfs_module 2>/dev/null
    exit 1
fi
echo ""

echo "4. Tao mount point va mount..."
mkdir -p ${MOUNT_POINT}
if mount -t simplefs none ${MOUNT_POINT} 2>/dev/null; then
    echo "   [OK] Filesystem da duoc mount tai ${MOUNT_POINT}"
else
    echo "   [FAIL] Khong the mount"
    rmmod vfs_module 2>/dev/null
    exit 1
fi
echo ""

echo "5. Liet ke cac file..."
ls -lh ${MOUNT_POINT}
echo ""

echo "6. Doc noi dung file 'hello'..."
echo "----------------------------"
cat ${MOUNT_POINT}/hello
echo "----------------------------"
echo ""

echo "7. Doc noi dung file 'info'..."
echo "----------------------------"
cat ${MOUNT_POINT}/info
echo "----------------------------"
echo ""

echo "8. Thong tin filesystem..."
echo "Mount point:"
mount | grep simplefs
echo ""
echo "Disk usage:"
df -h ${MOUNT_POINT}
echo ""

echo "9. Cleanup - Umount va unload module..."
if umount ${MOUNT_POINT} 2>/dev/null; then
    echo "   [OK] Umount thanh cong"
else
    echo "   [WARN] Umount that bai, thu force umount..."
    umount -f ${MOUNT_POINT} 2>/dev/null || umount -l ${MOUNT_POINT} 2>/dev/null
    sleep 1
fi

if rmmod vfs_module 2>/dev/null; then
    echo "   [OK] Module da duoc unload"
else
    echo "   [WARN] Khong the unload module"
    echo "   Thu force remove: rmmod -f vfs_module"
fi
echo ""

echo "=========================================="
echo "Demo hoan tat!"
echo "=========================================="
echo ""
echo "De xem kernel log: dmesg | grep simplefs"
echo "De chay test day du: sudo ./test_vfs.sh"
echo ""
