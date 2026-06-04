#!/bin/bash

# Script chứa các tác vụ định kỳ thường dùng
# Có thể được gọi từ cron hoặc systemd timer

set -e

LOG_DIR="/var/log/scheduled_tasks"
mkdir -p "$LOG_DIR"

# Màu sắc
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Hàm ghi log
log_message() {
    local level=$1
    shift
    local message="$@"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo "[$timestamp] [$level] $message" | tee -a "$LOG_DIR/tasks.log"
}

# Task 1: Backup tự động hàng ngày
daily_backup() {
    log_message "INFO" "Bắt đầu backup hàng ngày..."
    
    local backup_source="/home"
    local backup_dest="/backup/daily"
    local backup_name="backup-$(date +%Y%m%d).tar.gz"
    
    mkdir -p "$backup_dest"
    
    if tar -czf "$backup_dest/$backup_name" "$backup_source" 2>/dev/null; then
        log_message "INFO" "Backup thành công: $backup_dest/$backup_name"
        
        # Xóa backup cũ hơn 7 ngày
        find "$backup_dest" -name "backup-*.tar.gz" -mtime +7 -delete
        log_message "INFO" "Đã xóa các backup cũ hơn 7 ngày"
    else
        log_message "ERROR" "Backup thất bại!"
        return 1
    fi
}

# Task 2: Dọn dẹp file tạm
cleanup_temp_files() {
    log_message "INFO" "Bắt đầu dọn dẹp file tạm..."
    
    local temp_dirs=("/tmp" "/var/tmp")
    local days_old=3
    
    for dir in "${temp_dirs[@]}"; do
        if [ -d "$dir" ]; then
            local count=$(find "$dir" -type f -mtime +$days_old 2>/dev/null | wc -l)
            find "$dir" -type f -mtime +$days_old -delete 2>/dev/null
            log_message "INFO" "Đã xóa $count file(s) cũ hơn $days_old ngày từ $dir"
        fi
    done
}

# Task 3: Dọn dẹp log files
cleanup_logs() {
    log_message "INFO" "Bắt đầu dọn dẹp log files..."
    
    local log_dirs=("/var/log")
    local days_old=30
    
    for dir in "${log_dirs[@]}"; do
        if [ -d "$dir" ]; then
            # Xóa log files cũ
            find "$dir" -name "*.log" -mtime +$days_old -delete 2>/dev/null
            # Xóa log files đã nén
            find "$dir" -name "*.gz" -mtime +$days_old -delete 2>/dev/null
            log_message "INFO" "Đã dọn dẹp log files cũ hơn $days_old ngày từ $dir"
        fi
    done
}

# Task 4: Kiểm tra dung lượng đĩa
check_disk_space() {
    log_message "INFO" "Kiểm tra dung lượng đĩa..."
    
    local threshold=80
    
    df -h | grep -vE '^Filesystem|tmpfs|cdrom' | while read -r line; do
        local usage=$(echo "$line" | awk '{print $5}' | sed 's/%//')
        local partition=$(echo "$line" | awk '{print $6}')
        
        if [ "$usage" -ge "$threshold" ]; then
            log_message "WARNING" "Phân vùng $partition đang sử dụng ${usage}% dung lượng!"
        else
            log_message "INFO" "Phân vùng $partition: ${usage}% đã dùng"
        fi
    done
}

# Task 5: Update hệ thống
system_update() {
    log_message "INFO" "Bắt đầu update hệ thống..."
    
    if apt update >> "$LOG_DIR/tasks.log" 2>&1; then
        log_message "INFO" "Update danh sách package thành công"
        
        # Kiểm tra có update nào không
        local updates=$(apt list --upgradable 2>/dev/null | grep -c upgradable)
        
        if [ "$updates" -gt 1 ]; then
            log_message "INFO" "Có $((updates - 1)) package cần update"
        else
            log_message "INFO" "Hệ thống đã được cập nhật"
        fi
    else
        log_message "ERROR" "Update danh sách package thất bại!"
        return 1
    fi
}

# Task 6: Kiểm tra trạng thái dịch vụ
check_services() {
    log_message "INFO" "Kiểm tra trạng thái các dịch vụ quan trọng..."
    
    local services=("ssh" "cron")
    
    for service in "${services[@]}"; do
        if systemctl is-active --quiet "$service"; then
            log_message "INFO" "Dịch vụ $service đang chạy"
        else
            log_message "WARNING" "Dịch vụ $service KHÔNG chạy!"
        fi
    done
}

# Task 7: Rotate logs
rotate_logs() {
    log_message "INFO" "Rotate log files..."
    
    local log_file="$LOG_DIR/tasks.log"
    local max_size=10485760  # 10MB
    
    if [ -f "$log_file" ]; then
        local size=$(stat -f%z "$log_file" 2>/dev/null || stat -c%s "$log_file" 2>/dev/null)
        
        if [ "$size" -gt "$max_size" ]; then
            local timestamp=$(date +%Y%m%d-%H%M%S)
            mv "$log_file" "$log_file.$timestamp"
            gzip "$log_file.$timestamp"
            log_message "INFO" "Đã rotate log file"
            
            # Xóa log đã rotate cũ hơn 30 ngày
            find "$LOG_DIR" -name "tasks.log.*.gz" -mtime +30 -delete
        fi
    fi
}

# Hàm hiển thị usage
show_usage() {
    cat << EOF
Sử dụng: $0 <task_name>

Các tác vụ định kỳ có sẵn:
    daily_backup        - Backup hàng ngày
    cleanup_temp        - Dọn dẹp file tạm
    cleanup_logs        - Dọn dẹp log files cũ
    check_disk          - Kiểm tra dung lượng đĩa
    system_update       - Update hệ thống
    check_services      - Kiểm tra trạng thái dịch vụ
    rotate_logs         - Rotate log files
    all                 - Chạy tất cả các task

Ví dụ:
    $0 daily_backup
    $0 all
    
Sử dụng với cron:
    # Backup hàng ngày lúc 2 giờ sáng
    0 2 * * * /path/to/scheduled_tasks.sh daily_backup
    
    # Dọn dẹp mỗi tuần
    0 3 * * 0 /path/to/scheduled_tasks.sh cleanup_temp
EOF
}

# Main
if [ $# -eq 0 ]; then
    show_usage
    exit 0
fi

case "$1" in
    daily_backup)
        daily_backup
        ;;
    cleanup_temp)
        cleanup_temp_files
        ;;
    cleanup_logs)
        cleanup_logs
        ;;
    check_disk)
        check_disk_space
        ;;
    system_update)
        system_update
        ;;
    check_services)
        check_services
        ;;
    rotate_logs)
        rotate_logs
        ;;
    all)
        log_message "INFO" "========== Chạy tất cả các task định kỳ =========="
        check_disk_space
        check_services
        cleanup_temp_files
        cleanup_logs
        rotate_logs
        log_message "INFO" "========== Hoàn thành tất cả các task =========="
        ;;
    *)
        echo -e "${RED}Lỗi: Task không hợp lệ: $1${NC}"
        echo ""
        show_usage
        exit 1
        ;;
esac
