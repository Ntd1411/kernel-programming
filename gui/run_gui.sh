#!/bin/bash
# Script chạy GUI cho dự án Kernel Linux

# Lấy đường dẫn thư mục script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Kiểm tra Python3
if ! command -v python3 &> /dev/null; then
    echo "Lỗi: Python3 chưa được cài đặt"
    echo "Cài đặt bằng lệnh: sudo apt install python3"
    exit 1
fi

# Kiểm tra tkinter
if ! python3 -c "import tkinter" 2>/dev/null; then
    echo "Lỗi: Tkinter chưa được cài đặt"
    echo "Cài đặt bằng lệnh: sudo apt install python3-tk"
    exit 1
fi

# Chạy ứng dụng
echo "Đang khởi động Kernel Linux GUI..."
cd "$SCRIPT_DIR"
python3 app.py
