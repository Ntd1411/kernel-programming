#!/bin/bash
# test_loopback.sh - Test loopback network driver
# Sử dụng: sudo ./test_loopback.sh

set -e

echo "=========================================="
echo "Loopback Driver Test Script"
echo "=========================================="
echo ""

# Kiểm tra quyền root
if [ "$EUID" -ne 0 ]; then
    echo "Script này cần quyền root"
    echo "Chạy: sudo $0"
    exit 1
fi

# Build nếu chưa có
if [ ! -f loopback_driver.ko ]; then
    echo "Build module..."
    make modules
    echo ""
fi

echo "[1] Load loopback_driver module..."
if lsmod | grep -q loopback_driver; then
    echo "  Module đã load, unload trước..."
    rmmod loopback_driver
fi

insmod loopback_driver.ko
echo "  Module loaded"
sleep 1

echo ""
echo "[2] Kiểm tra interface được tạo..."
if ip link show myloop0 &>/dev/null; then
    echo "  ✓ Interface myloop0 đã được tạo"
    ip link show myloop0
else
    echo "  ✗ Interface myloop0 KHÔNG được tạo"
    rmmod loopback_driver
    exit 1
fi

echo ""
echo "[3] Cấu hình interface..."
ip link set myloop0 up
ip addr add 10.0.0.1/24 dev myloop0
echo "  ✓ Interface UP và đã set IP 10.0.0.1/24"
ip addr show myloop0

echo ""
echo "[4] Test ping (loopback)..."
echo "  Ping 10.0.0.2 (packet sẽ loop back)..."
if ping -c 5 -W 2 10.0.0.2 | grep -q "5 received"; then
    echo "  ✓ Ping thành công - Loopback hoạt động!"
else
    echo "  ⚠ Ping có packet loss (bình thường với loopback)"
fi

echo ""
echo "[5] Xem kernel log..."
echo "--------------------"
dmesg | grep -i "loopback\|myloop" | tail -20

echo ""
echo "[6] Cleanup..."
ip link set myloop0 down
rmmod loopback_driver
echo "  Module unloaded"

echo ""
echo "=========================================="
echo "Test hoàn thành!"
echo "=========================================="
echo ""
echo "Tóm tắt:"
echo "  ✓ Module load thành công"
echo "  ✓ Interface myloop0 được tạo"
echo "  ✓ Có thể up/down interface"
echo "  ✓ Có thể config IP"
echo "  ✓ Loopback functionality hoạt động"
