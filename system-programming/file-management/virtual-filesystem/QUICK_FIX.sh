#!/bin/bash

# QUICK_FIX.sh - Sửa nhanh lỗi "Module is in use"
# Chạy: sudo ./QUICK_FIX.sh

echo "=========================================="
echo "QUICK FIX - Module is in use"
echo "=========================================="
echo ""

if [ "$EUID" -ne 0 ]; then
    echo "Can quyen root! Chay: sudo ./QUICK_FIX.sh"
    exit 1
fi

MOUNT_POINT="/mnt/simplefs"

echo "Buoc 1: Force umount..."
umount -l ${MOUNT_POINT} 2>/dev/null
sleep 1
echo "Done"
echo ""

echo "Buoc 2: Kill processes dang su dung..."
fuser -km ${MOUNT_POINT} 2>/dev/null
sleep 1
echo "Done"
echo ""

echo "Buoc 3: Unload module..."
if rmmod vfs_module 2>/dev/null; then
    echo "SUCCESS! Module da duoc unload"
else
    echo "FAILED! Thu force remove..."
    if rmmod -f vfs_module 2>/dev/null; then
        echo "SUCCESS! Force remove thanh cong"
    else
        echo "FAILED! Can reboot: sudo reboot"
        exit 1
    fi
fi
echo ""

echo "Buoc 4: Kiem tra..."
if lsmod | grep -q vfs_module; then
    echo "CANH BAO: Module van con!"
    lsmod | grep vfs_module
else
    echo "OK! Module da bi xoa"
fi
echo ""

echo "=========================================="
echo "HOAN TAT! Bay gio ban co the:"
echo "  sudo ./demo_vfs.sh"
echo "=========================================="
