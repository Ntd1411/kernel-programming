#!/bin/bash
# test_http_detector.sh - Test HTTP password detector
# Sử dụng: sudo ./test_http_detector.sh

set -e

echo "=========================================="
echo "HTTP Password Detector Test Script"
echo "=========================================="
echo ""

# Kiểm tra quyền root
if [ "$EUID" -ne 0 ]; then
    echo "Script này cần quyền root"
    echo "Chạy: sudo $0"
    exit 1
fi

# Build nếu chưa có
if [ ! -f http_password_detector.ko ]; then
    echo "Build module..."
    make modules
    echo ""
fi

echo "[1] Load http_password_detector module..."
if lsmod | grep -q http_password_detector; then
    echo "  Module đã load, unload trước..."
    rmmod http_password_detector
fi

insmod http_password_detector.ko
echo "  Module loaded"
sleep 1

echo ""
echo "[2] Kiểm tra log file..."
LOG_FILE="/var/log/http_passwords.log"
if [ -f "$LOG_FILE" ]; then
    echo "  ✓ Log file đã được tạo: $LOG_FILE"
    ls -lh "$LOG_FILE"
else
    echo "  ⚠ Log file chưa tồn tại (sẽ tạo khi có password)"
fi

echo ""
echo "[3] Test với HTTP POST request..."
echo "  Test 1: Simple password..."
curl -X POST -d "username=admin&password=secret123" http://httpbin.org/post -m 5 -s > /dev/null 2>&1 &
sleep 2

echo "  Test 2: Multiple fields..."
curl -X POST -d "user=john&password=mypass456&email=test@example.com" http://httpbin.org/post -m 5 -s > /dev/null 2>&1 &
sleep 2

echo "  Test 3: Complex password..."
curl -X POST -d "login=admin&password=P@ssw0rd!2024&remember=true" http://httpbin.org/post -m 5 -s > /dev/null 2>&1 &
sleep 2

echo "  Đợi requests hoàn thành..."
sleep 5

echo ""
echo "[4] Xem kernel log..."
echo "--------------------"
dmesg | grep -i "password\|HTTP" | tail -20

echo ""
echo "[5] Xem log file..."
echo "--------------------"
if [ -f "$LOG_FILE" ]; then
    echo "Nội dung $LOG_FILE:"
    cat "$LOG_FILE"
    echo ""
    echo "Tổng số passwords detected: $(wc -l < "$LOG_FILE")"
else
    echo "⚠ Không tìm thấy log file"
    echo "  Có thể do:"
    echo "  - Không có HTTP traffic với password"
    echo "  - Firewall chặn outgoing traffic"
    echo "  - Chỉ bắt được HTTP (port 80), không bắt HTTPS"
fi

echo ""
echo "[6] Cleanup..."
rmmod http_password_detector
echo "  Module unloaded"

echo ""
echo "=========================================="
echo "Test hoàn thành!"
echo "=========================================="
echo ""
echo "Lưu ý:"
echo "  - Module chỉ bắt HTTP (port 80)"
echo "  - Không bắt được HTTPS (encrypted)"
echo "  - Log file: $LOG_FILE"
echo "  - Chỉ dùng cho mục đích học tập!"
