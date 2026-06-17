#!/bin/bash

# demo.sh - Demo tính năng chính của các shell scripts
# Chạy các ví dụ thực tế để showcase functionality

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEMO_DIR="$HOME/shell_demo_$(date +%s)"

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

print_header() {
    echo ""
    echo -e "${CYAN}========================================${NC}"
    echo -e "${CYAN}  $1${NC}"
    echo -e "${CYAN}========================================${NC}"
    echo ""
}

print_step() {
    echo -e "${BLUE}▶ $1${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_info() {
    echo -e "${YELLOW}ℹ $1${NC}"
}

pause() {
    echo ""
    read -p "Nhấn Enter để tiếp tục..." dummy
    echo ""
}

cleanup_demo() {
    print_info "Cleaning up demo directory: $DEMO_DIR"
    rm -rf "$DEMO_DIR"
}

# Trap để cleanup khi exit
trap cleanup_demo EXIT

print_header "Shell Scripting Demo - Kernel Programming Project"
echo "Demo directory: $DEMO_DIR"
echo ""
print_info "Sẽ demo các tính năng chính của từng module"

# Setup demo environment
mkdir -p "$DEMO_DIR"/{source,backup,temp}

#=============================================================================
# DEMO 1: File Management
#=============================================================================

print_header "DEMO 1: File Management"

print_step "1.1 Tạo test files"
for i in {1..3}; do
    echo "Content of file $i - $(date)" > "$DEMO_DIR/source/file$i.txt"
done
echo "Duplicate content" > "$DEMO_DIR/source/dup1.txt"
echo "Duplicate content" > "$DEMO_DIR/source/dup2.txt"
dd if=/dev/zero of="$DEMO_DIR/source/largefile.bin" bs=1M count=10 2>/dev/null
ls -lh "$DEMO_DIR/source/"
print_success "Created test files"
pause

print_step "1.2 Demo: Backup script"
"$SCRIPT_DIR/file-management/backup.sh" "$DEMO_DIR/source" "$DEMO_DIR/backup" 7
print_success "Backup completed"
ls -lh "$DEMO_DIR/backup/"
pause

print_step "1.3 Demo: Find duplicates"
"$SCRIPT_DIR/file-management/find_duplicates.sh" "$DEMO_DIR/source" list
print_success "Found duplicate files"
pause

print_step "1.4 Demo: Cleanup (dry-run)"
# Tạo temp files
touch "$DEMO_DIR/temp/temp_file.tmp"
echo "Testing cleanup in dry-run mode on demo directory:"
"$SCRIPT_DIR/file-management/cleanup.sh" -d 0 -n -v || true
print_success "Cleanup check completed (no files deleted)"
pause

#=============================================================================
# DEMO 2: Time Management
#=============================================================================

print_header "DEMO 2: Time Management"

print_step "2.1 Demo: Time Tracker - Current time and zones"
"$SCRIPT_DIR/time-management/time_tracker.sh" now
echo ""
"$SCRIPT_DIR/time-management/time_tracker.sh" zones
print_success "Time info displayed"
pause

print_step "2.2 Demo: Time Tracker - Uptime"
"$SCRIPT_DIR/time-management/time_tracker.sh" uptime
print_success "System uptime displayed"
pause

print_step "2.3 Demo: Time Tracker - Calendar"
"$SCRIPT_DIR/time-management/time_tracker.sh" calendar 6 2026
print_success "Calendar displayed"
pause

print_step "2.4 Demo: Time Tracker - Duration calculation"
"$SCRIPT_DIR/time-management/time_tracker.sh" duration "2026-12-31 23:59:59"
print_success "Duration calculated"
pause

print_step "2.5 Demo: Stopwatch - Quick demo (5 seconds)"
print_info "Starting stopwatch for 5 seconds..."
"$SCRIPT_DIR/time-management/stopwatch.sh" start "demo-task" &
sleep 2
"$SCRIPT_DIR/time-management/stopwatch.sh" lap
sleep 3
"$SCRIPT_DIR/time-management/stopwatch.sh" stop
print_success "Stopwatch demo completed"
pause

#=============================================================================
# DEMO 3: Package Management
#=============================================================================

print_header "DEMO 3: Package Management"

print_step "3.1 Demo: Search for a package"
"$SCRIPT_DIR/package-management/package_manager.sh" search vim | head -10
print_success "Package search completed"
pause

print_step "3.2 Demo: Check package dependencies"
"$SCRIPT_DIR/package-management/dependency_checker.sh" check bash 2>/dev/null || print_info "Note: Some commands may require package to be installed"
print_success "Dependency check completed"
pause

print_step "3.3 Demo: List repositories"
"$SCRIPT_DIR/package-management/repo_manager.sh" list | head -10
print_success "Repository list displayed"
pause

#=============================================================================
# DEMO 4: Task Scheduler
#=============================================================================

print_header "DEMO 4: Task Scheduler"

print_step "4.1 Demo: List current cron jobs"
"$SCRIPT_DIR/task-scheduler/cron_manager.sh" --list || print_info "No cron jobs or crontab not accessible"
print_success "Cron jobs listed"
pause

print_step "4.2 Demo: Scheduled tasks - Disk check"
sudo "$SCRIPT_DIR/task-scheduler/scheduled_tasks.sh" check_disk 2>/dev/null || \
    "$SCRIPT_DIR/task-scheduler/scheduled_tasks.sh" check_disk
print_success "Disk check completed"
pause

print_step "4.3 Demo: Scheduled tasks - Service check"
sudo "$SCRIPT_DIR/task-scheduler/scheduled_tasks.sh" check_services 2>/dev/null || \
    print_info "Service check requires sudo - skipped in demo"
pause

#=============================================================================
# DEMO 5: Advanced Features
#=============================================================================

print_header "DEMO 5: Advanced Features Demo"

print_step "5.1 Demo: Time conversion between zones"
"$SCRIPT_DIR/time-management/time_tracker.sh" convert "America/New_York" "Asia/Ho_Chi_Minh"
print_success "Timezone conversion completed"
pause

print_step "5.2 Demo: Add time to current date"
"$SCRIPT_DIR/time-management/time_tracker.sh" add now 7 days
"$SCRIPT_DIR/time-management/time_tracker.sh" add now 3 hours
print_success "Time addition completed"
pause

print_step "5.3 Demo: Backup verification"
if [ -f "$DEMO_DIR/backup"/backup_*.tar.gz ]; then
    backup_file=$(ls -t "$DEMO_DIR/backup"/backup_*.tar.gz | head -1)
    echo "Verifying backup: $(basename $backup_file)"
    tar -tzf "$backup_file" | head -10
    print_success "Backup verification successful"
else
    print_info "No backup file found"
fi
pause

print_step "5.4 Demo: File statistics"
echo "Source directory stats:"
du -sh "$DEMO_DIR/source"
echo ""
echo "File count by type:"
find "$DEMO_DIR/source" -type f -exec file {} \; | cut -d: -f2 | sort | uniq -c
print_success "File statistics displayed"
pause

#=============================================================================
# Summary
#=============================================================================

print_header "Demo Summary"

echo "Đã demo các tính năng:"
echo ""
echo "✓ File Management:"
echo "  - Backup tự động với retention policy"
echo "  - Tìm và xử lý file trùng lặp"
echo "  - Cleanup với dry-run mode"
echo ""
echo "✓ Time Management:"
echo "  - Hiển thị thời gian và múi giờ"
echo "  - Tính toán duration và convert timezone"
echo "  - Stopwatch và timer"
echo ""
echo "✓ Package Management:"
echo "  - Tìm kiếm packages"
echo "  - Kiểm tra dependencies"
echo "  - Quản lý repositories"
echo ""
echo "✓ Task Scheduler:"
echo "  - Quản lý cron jobs"
echo "  - Scheduled maintenance tasks"
echo ""

print_info "Demo directory sẽ tự động cleanup khi exit"
print_success "Demo hoàn thành!"

echo ""
echo "Để test chi tiết hơn, xem:"
echo "  - TEST_GUIDE.md: Hướng dẫn test đầy đủ"
echo "  - SETUP.md: Setup môi trường"
echo "  - README.md: Documentation đầy đủ"
echo ""
