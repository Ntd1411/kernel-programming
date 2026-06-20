#!/bin/bash

# demo_socket.sh - Script demo các chương trình socket
# 
# Sử dụng: ./demo_socket.sh

set -e

echo "=========================================="
echo "Socket Programming Demo Script"
echo "=========================================="
echo ""

# Kiểm tra các chương trình đã được build chưa
echo "[1/5] Kiểm tra build..."

if [ ! -f "./tcp_server" ] || [ ! -f "./tcp_client" ] || \
   [ ! -f "./udp_server" ] || [ ! -f "./udp_client" ] || \
   [ ! -f "./echo_server" ] || [ ! -f "./echo_client" ]; then
    echo "Các chương trình chưa được build. Đang build..."
    make clean
    make all
    echo "Build hoàn tất!"
else
    echo "Tất cả chương trình đã sẵn sàng."
fi

echo ""

# Menu chọn demo
while true; do
    echo "=========================================="
    echo "Chọn demo muốn chạy:"
    echo "=========================================="
    echo "1. Demo TCP Server/Client"
    echo "2. Demo UDP Server/Client"
    echo "3. Demo Echo Server/Client (Multi-threaded)"
    echo "4. Test với netcat"
    echo "5. Xem socket statistics"
    echo "0. Thoát"
    echo ""
    read -p "Lựa chọn: " choice
    
    case $choice in
        1)
            echo ""
            echo "=== Demo TCP Server/Client ==="
            echo ""
            echo "Hướng dẫn:"
            echo "- Mở terminal mới và chạy: ./tcp_client localhost 9001"
            echo "- Gõ tin nhắn và xem server echo lại"
            echo "- Nhấn Ctrl+C để dừng server"
            echo ""
            read -p "Nhấn Enter để khởi động TCP Server trên port 9001..."
            ./tcp_server 9001
            ;;
            
        2)
            echo ""
            echo "=== Demo UDP Server/Client ==="
            echo ""
            echo "Hướng dẫn:"
            echo "- Mở terminal mới và chạy: ./udp_client localhost 9002"
            echo "- Gửi datagram và xem response"
            echo "- Nhấn Ctrl+C để dừng server"
            echo ""
            read -p "Nhấn Enter để khởi động UDP Server trên port 9002..."
            ./udp_server 9002
            ;;
            
        3)
            echo ""
            echo "=== Demo Echo Server/Client (Multi-threaded) ==="
            echo ""
            echo "Hướng dẫn:"
            echo "- Mở nhiều terminal và chạy: ./echo_client localhost 9003"
            echo "- Mỗi client sẽ có thread riêng"
            echo "- Test với nhiều client đồng thời"
            echo "- Nhấn Ctrl+C để dừng server"
            echo ""
            read -p "Nhấn Enter để khởi động Echo Server trên port 9003..."
            ./echo_server 9003
            ;;
            
        4)
            echo ""
            echo "=== Test với netcat ==="
            echo ""
            echo "Option 1: Test TCP Server"
            echo "  Terminal 1: ./tcp_server 9004"
            echo "  Terminal 2: nc localhost 9004"
            echo ""
            echo "Option 2: Test UDP Server"
            echo "  Terminal 1: ./udp_server 9005"
            echo "  Terminal 2: nc -u localhost 9005"
            echo ""
            read -p "Nhấn Enter để tiếp tục..."
            ;;
            
        5)
            echo ""
            echo "=== Socket Statistics ==="
            echo ""
            echo "Các port đang listening:"
            if command -v ss &> /dev/null; then
                ss -tuln | grep -E '(State|LISTEN)'
            else
                netstat -tuln | grep -E '(Proto|LISTEN)'
            fi
            echo ""
            echo "Các kết nối đang active:"
            if command -v ss &> /dev/null; then
                ss -tan | head -20
            else
                netstat -tan | head -20
            fi
            echo ""
            read -p "Nhấn Enter để tiếp tục..."
            ;;
            
        0)
            echo ""
            echo "Thoát chương trình. Tạm biệt!"
            exit 0
            ;;
            
        *)
            echo ""
            echo "Lựa chọn không hợp lệ. Vui lòng chọn lại."
            echo ""
            ;;
    esac
    
    echo ""
done
