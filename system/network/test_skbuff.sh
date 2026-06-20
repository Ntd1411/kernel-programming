#!/bin/bash
# test_skbuff.sh - Test sk_buff demo module
# Sử dụng: sudo ./test_skbuff.sh

set -e

echo "=========================================="
echo "sk_buff Demo Test Script"
echo "=========================================="
echo ""

# Kiểm tra quyền root
if [ "$EUID" -ne 0 ]; then
    echo "Script này cần quyền root"
    echo "Chạy: sudo $0"
    exit 1
fi

# Build nếu chưa có
if [ ! -f skbuff_demo.ko ]; then
    echo "Build module..."
    make modules
    echo ""
fi

echo "[1] Clear kernel log để dễ đọc..."
dmesg -C

echo ""
echo "[2] Load skbuff_demo module..."
if lsmod | grep -q skbuff_demo; then
    echo "  Module đã load, unload trước..."
    rmmod skbuff_demo
fi

echo "  Loading module (tự động chạy 6 demos)..."
insmod skbuff_demo.ko
echo "  ✓ Module loaded và demos đã chạy"
sleep 1

echo ""
echo "[3] Xem kết quả các demos..."
echo "=========================================="
dmesg | tail -150

echo ""
echo "[4] Tóm tắt các demos đã chạy..."
echo "--------------------"
dmesg | grep "===" | tail -10

echo ""
echo "[5] Unload module..."
rmmod skbuff_demo
echo "  ✓ Module unloaded"

echo ""
echo "=========================================="
echo "Test hoàn thành!"
echo "=========================================="
echo ""
echo "Các demos đã chạy:"
echo "  1. Demo cấp phát sk_buff"
echo "  2. Demo thêm data (skb_put, skb_reserve)"
echo "  3. Demo push/pull headers"
echo "  4. Demo clone và copy sk_buff"
echo "  5. Demo phân tích packet structure"
echo "  6. Demo linear vs non-linear buffer"
echo ""
echo "Để xem lại chi tiết: dmesg | grep -E 'Demo|sk_buff|===' | less"
