#!/bin/bash
# Setup script for Shell Scripting GUI on Ubuntu 24.10
# Run this once to setup everything

set -e

echo "============================================"
echo "Shell Scripting GUI - Setup"
echo "============================================"
echo ""

# Check OS
if [[ ! -f /etc/os-release ]]; then
    echo "⚠️  Không phát hiện được OS. Script này được tối ưu cho Ubuntu 24.10"
fi

# Install Python3 and tkinter
echo "📦 Đang kiểm tra dependencies..."

if ! command -v python3 &> /dev/null; then
    echo "Cài đặt Python3..."
    sudo apt update
    sudo apt install -y python3
else
    echo "✅ Python3 đã được cài đặt: $(python3 --version)"
fi

if ! python3 -c "import tkinter" &> /dev/null 2>&1; then
    echo "Cài đặt tkinter..."
    sudo apt install -y python3-tk
else
    echo "✅ Tkinter đã được cài đặt"
fi

# Make all scripts executable
echo ""
echo "🔧 Đang cấp quyền thực thi cho scripts..."

chmod +x gui_launcher.py 2>/dev/null || true
chmod +x launch_gui.sh 2>/dev/null || true
chmod +x *.sh 2>/dev/null || true
chmod +x demo/*.sh 2>/dev/null || true
chmod +x file-management/*.sh 2>/dev/null || true
chmod +x time-management/*.sh 2>/dev/null || true
chmod +x package-management/*.sh 2>/dev/null || true
chmod +x task-scheduler/*.sh 2>/dev/null || true

echo "✅ Đã cấp quyền thực thi"

# Test GUI
echo ""
echo "🧪 Kiểm tra GUI..."
if python3 -c "
import tkinter as tk
import sys
try:
    root = tk.Tk()
    root.withdraw()
    print('✅ GUI test thành công!')
    root.destroy()
    sys.exit(0)
except Exception as e:
    print(f'❌ GUI test thất bại: {e}')
    sys.exit(1)
"; then
    echo ""
    echo "============================================"
    echo "✅ Setup hoàn tất!"
    echo "============================================"
    echo ""
    echo "Chạy GUI bằng một trong các cách sau:"
    echo "  1. ./launch_gui.sh"
    echo "  2. python3 gui_launcher.py"
    echo "  3. ./gui_launcher.py"
    echo ""
else
    echo ""
    echo "⚠️  Có vấn đề với GUI. Kiểm tra:"
    echo "  - Bạn có đang dùng desktop environment không?"
    echo "  - Nếu SSH, dùng: ssh -X user@host"
    exit 1
fi
