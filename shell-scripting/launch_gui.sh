#!/bin/bash
# Quick launcher for Shell Scripting GUI
# Usage: ./launch_gui.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GUI_SCRIPT="${SCRIPT_DIR}/gui_launcher.py"

# Check if Python3 is installed
if ! command -v python3 &> /dev/null; then
    echo "❌ Python3 không được cài đặt!"
    echo "Cài đặt: sudo apt install python3"
    exit 1
fi

# Check if tkinter is available
if ! python3 -c "import tkinter" &> /dev/null; then
    echo "❌ Tkinter không được cài đặt!"
    echo "Cài đặt: sudo apt install python3-tk"
    exit 1
fi

# Make GUI script executable
chmod +x "$GUI_SCRIPT"

# Launch GUI
echo "🚀 Đang khởi động Shell Scripting Launcher..."
python3 "$GUI_SCRIPT"
