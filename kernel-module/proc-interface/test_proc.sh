#!/bin/bash

#
# test_proc.sh - Script test proc interface modules
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG_FILE="test_proc.log"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log() {
    echo -e "${GREEN}[TEST]${NC} $1" | tee -a "$LOG_FILE"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1" | tee -a "$LOG_FILE"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $1" | tee -a "$LOG_FILE"
}

info() {
    echo -e "${BLUE}[INFO]${NC} $1" | tee -a "$LOG_FILE"
}

check_root() {
    if [ "$EUID" -ne 0 ]; then
        error "Vui long chay script voi quyen root (sudo)"
        exit 1
    fi
}

cleanup() {
    info "Cleanup..."
    rmmod proc_rw 2>/dev/null || true
    rmmod proc_seq 2>/dev/null || true
    rmmod proc_basic 2>/dev/null || true
}

test_proc_basic() {
    log "Test 1: Proc Basic Module"
    
    info "Load module proc_basic..."
    insmod proc_basic.ko
    
    if [ ! -e /proc/myproc ]; then
        error "/proc/myproc khong ton tai"
        return 1
    fi
    
    info "Doc du lieu tu /proc/myproc:"
    cat /proc/myproc
    
    info "Kiem tra permissions:"
    ls -l /proc/myproc
    
    info "Unload module..."
    rmmod proc_basic
    
    if [ -e /proc/myproc ]; then
        error "/proc/myproc van con ton tai sau khi unload"
        return 1
    fi
    
    log "Test 1: PASSED"
    return 0
}

test_proc_seq() {
    log "Test 2: Proc Sequential File"
    
    info "Load module proc_seq..."
    insmod proc_seq.ko
    
    if [ ! -e /proc/cpu_info ]; then
        error "/proc/cpu_info khong ton tai"
        return 1
    fi
    
    info "Doc du lieu tu /proc/cpu_info:"
    cat /proc/cpu_info
    
    info "Kiem tra permissions:"
    ls -l /proc/cpu_info
    
    info "Unload module..."
    rmmod proc_seq
    
    if [ -e /proc/cpu_info ]; then
        error "/proc/cpu_info van con ton tai sau khi unload"
        return 1
    fi
    
    log "Test 2: PASSED"
    return 0
}

test_proc_rw() {
    log "Test 3: Proc Read/Write"
    
    info "Load module proc_rw..."
    insmod proc_rw.ko
    
    if [ ! -e /proc/mymessage ]; then
        error "/proc/mymessage khong ton tai"
        return 1
    fi
    
    info "Doc message mac dinh:"
    cat /proc/mymessage
    
    info "Ghi message moi..."
    echo "Test message from script" > /proc/mymessage
    
    info "Doc lai message:"
    cat /proc/mymessage
    
    info "Test ghi message dai..."
    echo "This is a longer test message with multiple words and some special chars: !@#$%^&*()" > /proc/mymessage
    cat /proc/mymessage
    
    info "Test concurrent writes..."
    for i in {1..5}; do
        echo "Concurrent write $i" > /proc/mymessage &
    done
    wait
    
    cat /proc/mymessage
    
    info "Kiem tra permissions:"
    ls -l /proc/mymessage
    
    info "Unload module..."
    rmmod proc_rw
    
    if [ -e /proc/mymessage ]; then
        error "/proc/mymessage van con ton tai sau khi unload"
        return 1
    fi
    
    log "Test 3: PASSED"
    return 0
}

test_all_together() {
    log "Test 4: Load All Modules Together"
    
    info "Load tat ca modules..."
    insmod proc_basic.ko
    insmod proc_seq.ko
    insmod proc_rw.ko
    
    info "Kiem tra cac proc entries:"
    ls -l /proc/myproc /proc/cpu_info /proc/mymessage
    
    info "Test doc dong thoi:"
    cat /proc/myproc > /dev/null &
    cat /proc/cpu_info > /dev/null &
    cat /proc/mymessage > /dev/null &
    wait
    
    info "Unload tat ca modules..."
    rmmod proc_rw
    rmmod proc_seq
    rmmod proc_basic
    
    log "Test 4: PASSED"
    return 0
}

test_stress() {
    log "Test 5: Stress Test"
    
    info "Load modules..."
    insmod proc_basic.ko
    insmod proc_seq.ko
    insmod proc_rw.ko
    
    info "Stress test: nhieu lan doc..."
    for i in {1..100}; do
        cat /proc/myproc > /dev/null
        cat /proc/cpu_info > /dev/null
        cat /proc/mymessage > /dev/null
    done
    
    info "Stress test: nhieu lan ghi..."
    for i in {1..100}; do
        echo "Message $i" > /proc/mymessage
    done
    
    info "Stress test: concurrent access..."
    for i in {1..10}; do
        (
            for j in {1..10}; do
                cat /proc/myproc > /dev/null
                echo "Concurrent $i-$j" > /proc/mymessage
                cat /proc/cpu_info > /dev/null
            done
        ) &
    done
    wait
    
    info "Unload modules..."
    rmmod proc_rw
    rmmod proc_seq
    rmmod proc_basic
    
    log "Test 5: PASSED"
    return 0
}

show_kernel_log() {
    info "Kernel log lien quan:"
    dmesg | grep -i "proc" | tail -30
}

main() {
    log "=== BAT DAU TEST PROC INTERFACE MODULES ==="
    date | tee -a "$LOG_FILE"
    
    check_root
    
    cd "$SCRIPT_DIR"
    
    if [ ! -f "proc_basic.ko" ] || [ ! -f "proc_seq.ko" ] || [ ! -f "proc_rw.ko" ]; then
        error "Modules chua duoc compile. Chay 'make' truoc."
        exit 1
    fi
    
    cleanup
    
    FAILED=0
    
    test_proc_basic || FAILED=$((FAILED + 1))
    echo ""
    
    test_proc_seq || FAILED=$((FAILED + 1))
    echo ""
    
    test_proc_rw || FAILED=$((FAILED + 1))
    echo ""
    
    test_all_together || FAILED=$((FAILED + 1))
    echo ""
    
    test_stress || FAILED=$((FAILED + 1))
    echo ""
    
    show_kernel_log
    echo ""
    
    cleanup
    
    log "=== KET THUC TEST ==="
    
    if [ $FAILED -eq 0 ]; then
        log "TAT CA TEST PASSED"
        exit 0
    else
        error "$FAILED TEST FAILED"
        exit 1
    fi
}

main "$@"
