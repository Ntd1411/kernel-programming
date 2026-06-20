#!/bin/bash
# test_vfs_tutorial.sh - Test simple VFS module

set -e

MODULE="vfs_module"
MOUNT_PT="/mnt/vfs_demo"

if [ "$EUID" -ne 0 ]; then
    echo "[!] Script này cần root/sudo"
    exit 1
fi

echo "[*] Clean up previous state..."
umount "$MOUNT_PT" 2>/dev/null || true
rmmod "$MODULE" 2>/dev/null || true
sleep 1

echo "[*] Build module..."
make -f Makefile.tutorial clean
make -f Makefile.tutorial

echo "[*] Load module..."
insmod "${MODULE}.ko"
sleep 1

echo "[*] Check dmesg (module init)..."
dmesg | tail -15

echo "[*] Create mount point..."
mkdir -p "$MOUNT_PT"

echo "[*] Mount filesystem..."
mount -t vfs_demo none "$MOUNT_PT"
sleep 1

echo "[*] Check dmesg (mount)..."
dmesg | tail -10

echo "[*] List directory..."
ls -la "$MOUNT_PT"

echo "[*] Read file..."
cat "$MOUNT_PT/hello"

echo "[*] Umount..."
umount "$MOUNT_PT"
sleep 1

echo "[*] Check dmesg (umount)..."
dmesg | tail -10

echo "[*] Unload module..."
rmmod "$MODULE"
sleep 1

echo "[*] Check dmesg (module exit)..."
dmesg | tail -10

echo ""
echo "[✓] Test completed successfully!"