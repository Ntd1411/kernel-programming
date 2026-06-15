#!/bin/bash

#
# demo_proc.sh - Script demo proc interface modules
#

set -e

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

info() {
    echo -e "${BLUE}>>> $1${NC}"
}

cmd() {
    echo -e "${YELLOW}$ $1${NC}"
    eval "$1"
}

pause() {
    echo ""
    read -p "Nhan Enter de tiep tuc..."
    echo ""
}

check_root() {
    if [ "$EUID" -ne 0 ]; then
        echo "Vui long chay script voi quyen root: sudo $0"
        exit 1
    fi
}

main() {
    check_root
    
    echo "========================================"
    echo "  DEMO PROC INTERFACE MODULES"
    echo "========================================"
    echo ""
    
    info "Build modules..."
    cmd "make clean && make"
    pause
    
    # Demo 1: Proc Basic
    info "DEMO 1: Proc Basic - Read-only proc entry"
    cmd "insmod proc_basic.ko"
    cmd "ls -l /proc/myproc"
    echo ""
    info "Doc thong tin CPU tu /proc/myproc:"
    cmd "cat /proc/myproc"
    echo ""
    info "Kiem tra kernel log:"
    cmd "dmesg | grep ProcBasic | tail -5"
    pause
    
    cmd "rmmod proc_basic"
    info "Da remove proc_basic module"
    pause
    
    # Demo 2: Proc Seq
    info "DEMO 2: Proc Sequential File - Danh sach CPU"
    cmd "insmod proc_seq.ko"
    cmd "ls -l /proc/cpu_info"
    echo ""
    info "Hien thi thong tin tat ca CPU:"
    cmd "cat /proc/cpu_info"
    echo ""
    info "Kiem tra kernel log:"
    cmd "dmesg | grep ProcSeq | tail -5"
    pause
    
    cmd "rmmod proc_seq"
    info "Da remove proc_seq module"
    pause
    
    # Demo 3: Proc RW
    info "DEMO 3: Proc Read/Write - Message buffer"
    cmd "insmod proc_rw.ko"
    cmd "ls -l /proc/mymessage"
    echo ""
    info "Doc message mac dinh:"
    cmd "cat /proc/mymessage"
    pause
    
    info "Ghi message moi vao buffer:"
    cmd "echo 'Hello from demo script!' > /proc/mymessage"
    echo ""
    info "Doc lai message:"
    cmd "cat /proc/mymessage"
    pause
    
    info "Ghi message dai hon:"
    cmd "echo 'This is a longer message with special chars: !@#$%^&*()' > /proc/mymessage"
    echo ""
    info "Doc lai:"
    cmd "cat /proc/mymessage"
    echo ""
    info "Kiem tra kernel log:"
    cmd "dmesg | grep ProcRW | tail -10"
    pause
    
    cmd "rmmod proc_rw"
    info "Da remove proc_rw module"
    pause
    
    # Demo 4: Tat ca cung luc
    info "DEMO 4: Load tat ca modules cung luc"
    cmd "insmod proc_basic.ko"
    cmd "insmod proc_seq.ko"
    cmd "insmod proc_rw.ko"
    echo ""
    info "Kiem tra cac proc entries:"
    cmd "ls -l /proc/myproc /proc/cpu_info /proc/mymessage"
    echo ""
    info "Test doc dong thoi:"
    cmd "cat /proc/myproc"
    echo ""
    cmd "cat /proc/cpu_info"
    echo ""
    cmd "cat /proc/mymessage"
    pause
    
    info "Unload tat ca modules:"
    cmd "rmmod proc_rw"
    cmd "rmmod proc_seq"
    cmd "rmmod proc_basic"
    echo ""
    info "Xac nhan da remove:"
    cmd "lsmod | grep proc || echo 'Khong con module proc nao'"
    pause
    
    # Cleanup
    info "Cleanup..."
    cmd "make clean"
    
    echo ""
    echo "========================================"
    echo "  DEMO HOAN THANH!"
    echo "========================================"
}

main "$@"
