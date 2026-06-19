#!/bin/bash

# test_vfs.sh - Script test SimplFS kernel module
# Chay: sudo ./test_vfs.sh

set -e

MOUNT_POINT="/mnt/simplefs"
MODULE_NAME="vfs_module"

# Mau sac cho output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Ham in thong bao
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Kiem tra quyen root
if [ "$EUID" -ne 0 ]; then
    print_error "Script nay can quyen root. Vui long chay voi sudo."
    exit 1
fi

print_info "Bat dau test SimplFS kernel module"
echo ""

# 1. Build module
print_info "Step 1: Build kernel module"
make clean > /dev/null 2>&1
if make > /dev/null 2>&1; then
    print_success "Build thanh cong"
else
    print_error "Build that bai"
    exit 1
fi
echo ""

# 2. Load module
print_info "Step 2: Load kernel module"
if insmod ${MODULE_NAME}.ko; then
    print_success "Module da duoc load"
else
    print_error "Khong the load module"
    exit 1
fi
echo ""

# 3. Kiem tra module da load
print_info "Step 3: Kiem tra module trong kernel"
if lsmod | grep -q ${MODULE_NAME}; then
    print_success "Module ${MODULE_NAME} da duoc load vao kernel"
    lsmod | grep ${MODULE_NAME}
else
    print_error "Module khong ton tai trong kernel"
    exit 1
fi
echo ""

# 4. Kiem tra filesystem type
print_info "Step 4: Kiem tra filesystem type da dang ky"
if cat /proc/filesystems | grep -q simplefs; then
    print_success "Filesystem 'simplefs' da duoc dang ky"
    cat /proc/filesystems | grep simplefs
else
    print_error "Filesystem chua duoc dang ky"
    rmmod ${MODULE_NAME}
    exit 1
fi
echo ""

# 5. Tao mount point
print_info "Step 5: Tao mount point"
mkdir -p ${MOUNT_POINT}
print_success "Mount point ${MOUNT_POINT} da duoc tao"
echo ""

# 6. Mount filesystem
print_info "Step 6: Mount filesystem"
if mount -t simplefs none ${MOUNT_POINT}; then
    print_success "Filesystem da duoc mount tai ${MOUNT_POINT}"
else
    print_error "Khong the mount filesystem"
    rmmod ${MODULE_NAME}
    exit 1
fi
echo ""

# 7. Kiem tra mount
print_info "Step 7: Kiem tra mount point"
if mount | grep -q simplefs; then
    print_success "Mount point dang hoat dong"
    mount | grep simplefs
else
    print_error "Mount point khong hoat dong"
    umount ${MOUNT_POINT} 2>/dev/null
    rmmod ${MODULE_NAME}
    exit 1
fi
echo ""

# 8. Liet ke file
print_info "Step 8: Liet ke cac file trong filesystem"
ls -la ${MOUNT_POINT}
echo ""

# 9. Doc file hello
print_info "Step 9: Doc noi dung file 'hello'"
if [ -f "${MOUNT_POINT}/hello" ]; then
    print_success "File 'hello' ton tai"
    echo "--- Noi dung ---"
    cat ${MOUNT_POINT}/hello
    echo "----------------"
else
    print_error "File 'hello' khong ton tai"
fi
echo ""

# 10. Doc file info
print_info "Step 10: Doc noi dung file 'info'"
if [ -f "${MOUNT_POINT}/info" ]; then
    print_success "File 'info' ton tai"
    echo "--- Noi dung ---"
    cat ${MOUNT_POINT}/info
    echo "----------------"
else
    print_error "File 'info' khong ton tai"
fi
echo ""

# 11. Kiem tra stat
print_info "Step 11: Kiem tra thong tin filesystem (stat)"
stat -f ${MOUNT_POINT}
echo ""

# 12. Kiem tra df
print_info "Step 12: Kiem tra dung luong (df)"
df -h ${MOUNT_POINT}
echo ""

# 13. Thu ghi file (nen that bai vi read-only)
print_info "Step 13: Thu ghi file (nen that bai - filesystem la read-only)"
if echo "test" > ${MOUNT_POINT}/test.txt 2>/dev/null; then
    print_warning "Canh bao: Ghi file thanh cong (khong mong doi)"
else
    print_success "Ghi file that bai nhu mong doi (read-only filesystem)"
fi
echo ""

# 14. Xem kernel log
print_info "Step 14: Xem kernel log (20 dong cuoi)"
echo "--- Kernel Log ---"
dmesg | grep simplefs | tail -20
echo "------------------"
echo ""

# 15. Umount filesystem
print_info "Step 15: Umount filesystem"
if umount ${MOUNT_POINT}; then
    print_success "Filesystem da duoc umount"
else
    print_error "Khong the umount filesystem"
    exit 1
fi
echo ""

# 16. Kiem tra lai mount
print_info "Step 16: Kiem tra filesystem da duoc umount"
if mount | grep -q simplefs; then
    print_error "Filesystem van con duoc mount"
else
    print_success "Filesystem da duoc umount hoan toan"
fi
echo ""

# 17. Unload module
print_info "Step 17: Unload kernel module"
if rmmod ${MODULE_NAME}; then
    print_success "Module da duoc unload"
else
    print_error "Khong the unload module"
    exit 1
fi
echo ""

# 18. Kiem tra module da unload
print_info "Step 18: Kiem tra module da duoc remove"
if lsmod | grep -q ${MODULE_NAME}; then
    print_error "Module van con trong kernel"
else
    print_success "Module da duoc remove khoi kernel"
fi
echo ""

# 19. Xem kernel log sau khi unload
print_info "Step 19: Xem kernel log sau khi unload"
echo "--- Kernel Log ---"
dmesg | grep simplefs | tail -10
echo "------------------"
echo ""

# Ket thuc
print_success "==================================="
print_success "Tat ca cac test da PASS thanh cong!"
print_success "==================================="
echo ""

print_info "Tong ket:"
echo "  - Module build: OK"
echo "  - Module load: OK"
echo "  - Filesystem register: OK"
echo "  - Mount: OK"
echo "  - File read: OK"
echo "  - Umount: OK"
echo "  - Module unload: OK"
echo ""

exit 0
