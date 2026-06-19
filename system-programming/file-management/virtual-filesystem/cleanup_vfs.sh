#!/bin/bash

# cleanup_vfs.sh - Script dọn dẹp SimplFS khi gặp vấn đề
# Chạy: sudo ./cleanup_vfs.sh

MOUNT_POINT="/mnt/simplefs"

echo "=========================================="
echo "SimplFS Cleanup Script"
echo "=========================================="
echo ""

# Kiểm tra quyền root
if [ "$EUID" -ne 0 ]; then
    echo "Loi: Script nay can quyen root"
    echo "Vui long chay: sudo ./cleanup_vfs.sh"
    exit 1
fi

echo "1. Kiem tra mount points..."
if mount | grep -q simplefs; then
    echo "   [FOUND] SimplFS dang duoc mount"
    mount | grep simplefs
    echo ""
    echo "   Dang umount..."
    
    # Thu umount binh thuong
    if umount ${MOUNT_POINT} 2>/dev/null; then
        echo "   [OK] Umount thanh cong"
    else
        echo "   [WARN] Umount binh thuong that bai, thu force..."
        
        # Thu force umount
        if umount -f ${MOUNT_POINT} 2>/dev/null; then
            echo "   [OK] Force umount thanh cong"
        else
            echo "   [WARN] Force umount that bai, thu lazy umount..."
            
            # Thu lazy umount
            if umount -l ${MOUNT_POINT} 2>/dev/null; then
                echo "   [OK] Lazy umount thanh cong"
            else
                echo "   [FAIL] Tat ca cac phuong phap umount deu that bai"
            fi
        fi
    fi
    sleep 1
else
    echo "   [OK] Khong co mount point nao"
fi
echo ""

echo "2. Kiem tra module vfs_module..."
if lsmod | grep -q "^vfs_module"; then
    echo "   [FOUND] Module vfs_module dang duoc load"
    lsmod | grep vfs_module
    echo ""
    echo "   Dang unload module..."
    
    # Thu rmmod binh thuong
    if rmmod vfs_module 2>/dev/null; then
        echo "   [OK] Unload thanh cong"
    else
        echo "   [WARN] Unload binh thuong that bai"
        echo ""
        echo "   Kiem tra module co dang duoc su dung..."
        lsmod | grep vfs_module
        echo ""
        
        # Hien thi cac process dang su dung
        echo "   Cac process dang su dung mount point:"
        lsof ${MOUNT_POINT} 2>/dev/null || fuser -m ${MOUNT_POINT} 2>/dev/null
        echo ""
        
        # Hoi nguoi dung co muon kill processes khong
        read -p "   Ban co muon kill cac process dang su dung? (y/n): " answer
        if [ "$answer" = "y" ]; then
            fuser -km ${MOUNT_POINT} 2>/dev/null
            sleep 2
            
            # Thu lai
            if rmmod vfs_module 2>/dev/null; then
                echo "   [OK] Unload thanh cong sau khi kill processes"
            else
                echo "   [FAIL] Van khong the unload"
                echo ""
                echo "   Thu force remove..."
                if rmmod -f vfs_module 2>/dev/null; then
                    echo "   [OK] Force remove thanh cong"
                else
                    echo "   [FAIL] Force remove that bai"
                    echo ""
                    echo "   Ban co the can:"
                    echo "   1. Reboot he thong: sudo reboot"
                    echo "   2. Hoac doi module tu dong unload"
                fi
            fi
        else
            echo "   [SKIP] Khong kill processes"
            echo ""
            echo "   De unload module, ban can:"
            echo "   1. Dong tat ca terminals dang o trong ${MOUNT_POINT}"
            echo "   2. Kill cac process: fuser -km ${MOUNT_POINT}"
            echo "   3. Thu lai: rmmod vfs_module"
        fi
    fi
else
    echo "   [OK] Module khong duoc load"
fi
echo ""

echo "3. Kiem tra mount point directory..."
if [ -d ${MOUNT_POINT} ]; then
    echo "   [FOUND] Mount point directory ton tai"
    echo "   Giu nguyen directory cho lan su dung sau"
else
    echo "   [OK] Mount point directory khong ton tai"
fi
echo ""

echo "=========================================="
echo "Cleanup hoan tat!"
echo "=========================================="
echo ""

# Hien thi trang thai cuoi cung
echo "Trang thai cuoi cung:"
echo "- Module loaded: $(lsmod | grep -q '^vfs_module' && echo 'YES' || echo 'NO')"
echo "- Mounted: $(mount | grep -q simplefs && echo 'YES' || echo 'NO')"
echo ""
echo "Ban co the chay lai demo: sudo ./demo_vfs.sh"
echo ""
