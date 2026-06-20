#!/bin/bash
# test_steganography.sh - Script test TCP/UDP Steganography
# 
# Tự động test module ẩn tin và đọc tin
# Sử dụng: sudo ./test_steganography.sh

set -e

echo "=========================================="
echo "TCP/UDP Steganography Test Script"
echo "=========================================="
echo ""

# Kiểm tra quyền root
if [ "$EUID" -ne 0 ]; then
    echo "Script này cần quyền root"
    echo "Chạy: sudo $0"
    exit 1
fi

# Biến cấu hình
INTERFACE="eth0"
MESSAGE="HELLO_SECRET_WORLD"
TEST_DURATION=15

echo "Cấu hình:"
echo "  Interface: $INTERFACE"
echo "  Message: $MESSAGE"
echo "  Test duration: ${TEST_DURATION}s"
echo ""

# Kiểm tra interface tồn tại
if ! ip link show "$INTERFACE" &>/dev/null; then
    echo "Cảnh báo: Interface $INTERFACE không tồn tại"
    echo "Các interface có sẵn:"
    ip link show | grep '^[0-9]' | cut -d: -f2
    read -p "Nhập tên interface: " INTERFACE
fi

# Build nếu chưa có
if [ ! -f tcp_steganography.ko ] || [ ! -f stego_reader ]; then
    echo "Build modules và programs..."
    make
    echo ""
fi

# Load module
echo "[1] Load tcp_steganography module..."
if lsmod | grep -q tcp_steganography; then
    echo "  Module đã load, unload trước..."
    rmmod tcp_steganography
fi

insmod tcp_steganography.ko message="$MESSAGE"
echo "  Module loaded với message: $MESSAGE"
sleep 1

# Chạy stego_reader trong background
echo ""
echo "[2] Khởi động stego_reader trong background..."
READER_LOG="/tmp/stego_reader.log"
./stego_reader "$INTERFACE" > "$READER_LOG" 2>&1 &
READER_PID=$!
echo "  Reader PID: $READER_PID"
echo "  Log file: $READER_LOG"
sleep 2

# Tạo traffic
echo ""
echo "[3] Tạo network traffic..."
echo "  Ping google.com..."
ping -c 10 google.com >/dev/null 2>&1 &

echo "  Curl example.com..."
curl -s http://example.com >/dev/null 2>&1 &

echo "  Đợi $TEST_DURATION giây để thu thập data..."
sleep $TEST_DURATION

# Dừng reader
echo ""
echo "[4] Dừng stego_reader..."
kill -INT $READER_PID 2>/dev/null || true
sleep 2

# Hiển thị kết quả
echo ""
echo "=========================================="
echo "KẾT QUẢ"
echo "=========================================="
echo ""

echo "Log từ stego_reader:"
echo "--------------------"
if [ -f "$READER_LOG" ]; then
    tail -100 "$READER_LOG"
else
    echo "Không tìm thấy log file"
fi

echo ""
echo "Kernel log (dmesg):"
echo "-------------------"
dmesg | grep -i stego | tail -20

# Unload module
echo ""
echo "[5] Unload module..."
rmmod tcp_steganography
echo "  Module unloaded"

# Cleanup
rm -f "$READER_LOG"

echo ""
echo "=========================================="
echo "Test hoàn thành!"
echo "=========================================="