#!/bin/bash

# test_socket.sh - Script tự động test socket programs
# 
# Sử dụng: ./test_socket.sh

set -e

TIMEOUT=5
TEST_PORT_BASE=9100

echo "=========================================="
echo "Socket Programming Automated Tests"
echo "=========================================="
echo ""

# Build trước
echo "[1] Building programs..."
make clean > /dev/null 2>&1
make all > /dev/null 2>&1
echo "Build completed."
echo ""

# Test 1: TCP Server/Client
echo "[2] Testing TCP Server/Client..."
TEST_PORT=$((TEST_PORT_BASE + 1))

# Khởi động server ở background
./tcp_server $TEST_PORT > /tmp/tcp_server.log 2>&1 &
SERVER_PID=$!
sleep 1

# Kiểm tra server có chạy không
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "  [FAILED] TCP Server không khởi động được"
    cat /tmp/tcp_server.log
else
    echo "  [OK] TCP Server đã khởi động (PID: $SERVER_PID, Port: $TEST_PORT)"
    
    # Test với echo command
    echo "test message" | timeout $TIMEOUT ./tcp_client localhost $TEST_PORT > /tmp/tcp_client.log 2>&1
    
    if grep -q "test message" /tmp/tcp_client.log; then
        echo "  [OK] TCP Client gửi/nhận thành công"
    else
        echo "  [FAILED] TCP Client không nhận được echo"
    fi
    
    # Dừng server
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    echo "  [OK] TCP Server đã dừng"
fi
echo ""

# Test 2: UDP Server/Client
echo "[3] Testing UDP Server/Client..."
TEST_PORT=$((TEST_PORT_BASE + 2))

# Khởi động UDP server
./udp_server $TEST_PORT > /tmp/udp_server.log 2>&1 &
SERVER_PID=$!
sleep 1

if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "  [FAILED] UDP Server không khởi động được"
    cat /tmp/udp_server.log
else
    echo "  [OK] UDP Server đã khởi động (PID: $SERVER_PID, Port: $TEST_PORT)"
    
    # Test với echo command
    echo "udp test" | timeout $TIMEOUT ./udp_client localhost $TEST_PORT > /tmp/udp_client.log 2>&1
    
    if grep -q "udp test" /tmp/udp_client.log; then
        echo "  [OK] UDP Client gửi/nhận thành công"
    else
        echo "  [FAILED] UDP Client không nhận được response"
    fi
    
    # Dừng server
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    echo "  [OK] UDP Server đã dừng"
fi
echo ""

# Test 3: Echo Server với nhiều clients
echo "[4] Testing Multi-threaded Echo Server..."
TEST_PORT=$((TEST_PORT_BASE + 3))

# Khởi động echo server
./echo_server $TEST_PORT > /tmp/echo_server.log 2>&1 &
SERVER_PID=$!
sleep 1

if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "  [FAILED] Echo Server không khởi động được"
    cat /tmp/echo_server.log
else
    echo "  [OK] Echo Server đã khởi động (PID: $SERVER_PID, Port: $TEST_PORT)"
    
    # Test với 3 clients đồng thời
    for i in 1 2 3; do
        (echo "client $i message" | timeout $TIMEOUT ./echo_client localhost $TEST_PORT > /tmp/echo_client_$i.log 2>&1) &
        CLIENT_PIDS[$i]=$!
    done
    
    # Đợi tất cả clients
    sleep 2
    
    # Kiểm tra kết quả
    SUCCESS=0
    for i in 1 2 3; do
        if grep -q "client $i message" /tmp/echo_client_$i.log 2>/dev/null; then
            ((SUCCESS++))
        fi
    done
    
    echo "  [OK] Echo Server xử lý được $SUCCESS/3 clients đồng thời"
    
    if [ $SUCCESS -eq 3 ]; then
        echo "  [OK] Multi-threading hoạt động tốt"
    else
        echo "  [WARNING] Một số clients thất bại"
    fi
    
    # Dừng server
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    echo "  [OK] Echo Server đã dừng"
fi
echo ""

# Test 4: Kiểm tra port reuse
echo "[5] Testing SO_REUSEADDR..."
TEST_PORT=$((TEST_PORT_BASE + 4))

./tcp_server $TEST_PORT > /tmp/test_reuse1.log 2>&1 &
PID1=$!
sleep 1
kill $PID1 2>/dev/null || true
wait $PID1 2>/dev/null || true

# Ngay lập tức khởi động lại
./tcp_server $TEST_PORT > /tmp/test_reuse2.log 2>&1 &
PID2=$!
sleep 1

if kill -0 $PID2 2>/dev/null; then
    echo "  [OK] SO_REUSEADDR hoạt động - có thể reuse port ngay"
    kill $PID2 2>/dev/null || true
    wait $PID2 2>/dev/null || true
else
    echo "  [FAILED] Không thể reuse port ngay lập tức"
fi
echo ""

# Cleanup
echo "[6] Cleaning up..."
rm -f /tmp/tcp_*.log /tmp/udp_*.log /tmp/echo_*.log /tmp/test_*.log
echo "  [OK] Đã xóa log files"
echo ""

echo "=========================================="
echo "Test Summary"
echo "=========================================="
echo "Tất cả tests đã hoàn tất!"
echo ""
echo "Để chạy manual tests, sử dụng:"
echo "  ./demo_socket.sh"
echo ""
