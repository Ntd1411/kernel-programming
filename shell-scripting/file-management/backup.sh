#!/bin/bash

# backup.sh - Script backup tự động
# Sử dụng: ./backup.sh <source_dir> <backup_dir> [retention_days]

set -e

if [ $# -lt 2 ]; then
    echo "Sử dụng: $0 <source_dir> <backup_dir> [retention_days]"
    echo "Ví dụ: $0 /home/user /backup 7"
    exit 1
fi

SOURCE_DIR="$1"
BACKUP_DIR="$2"
RETENTION_DAYS="${3:-7}"

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
HOSTNAME=$(hostname)
BACKUP_NAME="backup_${HOSTNAME}_${TIMESTAMP}.tar.gz"
BACKUP_PATH="${BACKUP_DIR}/${BACKUP_NAME}"
LOG_FILE="${BACKUP_DIR}/backup.log"

log_message() {
    mkdir -p "$(dirname "$LOG_FILE")"
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

check_dependencies() {
    for cmd in tar gzip find; do
        if ! command -v $cmd &> /dev/null; then
            log_message "LỖI: Thiếu lệnh $cmd"
            exit 1
        fi
    done
}

check_source() {
    if [ ! -d "$SOURCE_DIR" ]; then
        log_message "LỖI: Thư mục nguồn không tồn tại: $SOURCE_DIR"
        exit 1
    fi
    
    if [ ! -r "$SOURCE_DIR" ]; then
        log_message "LỖI: Không có quyền đọc: $SOURCE_DIR"
        exit 1
    fi
}

prepare_backup_dir() {
    if [ ! -d "$BACKUP_DIR" ]; then
        log_message "Tạo thư mục backup: $BACKUP_DIR"
        mkdir -p "$BACKUP_DIR"
    fi
}

calculate_size() {
    du -sh "$SOURCE_DIR" | cut -f1
}

do_backup() {
    local size=$(calculate_size)
    
    log_message "==================== BẮT ĐẦU BACKUP ===================="
    log_message "Nguồn: $SOURCE_DIR (Size: $size)"
    log_message "Đích: $BACKUP_PATH"
    log_message "Retention: $RETENTION_DAYS ngày"
    
    local start_time=$(date +%s)
    
    if tar -czf "$BACKUP_PATH" -C "$(dirname "$SOURCE_DIR")" "$(basename "$SOURCE_DIR")" 2>&1 | tee -a "$LOG_FILE"; then
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        local backup_size=$(du -h "$BACKUP_PATH" | cut -f1)
        
        log_message "Backup thành công!"
        log_message "File backup: $BACKUP_PATH"
        log_message "Kích thước: $backup_size"
        log_message "Thời gian: ${duration}s"
        log_message "MD5: $(md5sum "$BACKUP_PATH" | cut -d' ' -f1)"
        
        return 0
    else
        log_message "LỖI: Backup thất bại!"
        [ -f "$BACKUP_PATH" ] && rm -f "$BACKUP_PATH"
        return 1
    fi
}

cleanup_old_backups() {
    log_message "Dọn dẹp các backup cũ hơn $RETENTION_DAYS ngày..."
    
    local count=$(find "$BACKUP_DIR" -name "backup_*.tar.gz" -type f -mtime +$RETENTION_DAYS | wc -l)
    
    if [ $count -gt 0 ]; then
        find "$BACKUP_DIR" -name "backup_*.tar.gz" -type f -mtime +$RETENTION_DAYS -print -delete | while read -r file; do
            log_message "Xóa: $file"
        done
        log_message "Đã xóa $count backup cũ"
    else
        log_message "Không có backup cũ cần xóa"
    fi
}

list_backups() {
    log_message "Danh sách các backup hiện có:"
    
    find "$BACKUP_DIR" -name "backup_*.tar.gz" -type f -printf "%T@ %p\n" | \
        sort -rn | \
        awk '{print $2}' | \
        while read -r backup; do
            local size=$(du -h "$backup" | cut -f1)
            local date=$(stat -c %y "$backup" | cut -d'.' -f1)
            log_message "  - $(basename "$backup") ($size) - $date"
        done
}

verify_backup() {
    log_message "Xác minh backup..."
    
    if tar -tzf "$BACKUP_PATH" > /dev/null 2>&1; then
        log_message "Backup hợp lệ"
        return 0
    else
        log_message "LỖI: Backup bị hỏng!"
        return 1
    fi
}

send_notification() {
    local status=$1
    local message=$2
    
    if [ "$status" = "success" ]; then
        echo "Backup hoàn thành: $message" | mail -s "Backup Success - $HOSTNAME" root 2>/dev/null || true
    else
        echo "Backup thất bại: $message" | mail -s "Backup Failed - $HOSTNAME" root 2>/dev/null || true
    fi
}

main() {
    log_message "==================== BACKUP SCRIPT START ===================="
    
    check_dependencies
    check_source
    prepare_backup_dir
    
    if do_backup; then
        if verify_backup; then
            cleanup_old_backups
            list_backups
            
            log_message "==================== BACKUP HOÀN THÀNH ===================="
            send_notification "success" "$BACKUP_NAME"
            exit 0
        else
            log_message "==================== BACKUP FAILED (VERIFY) ===================="
            send_notification "failed" "Backup verification failed"
            exit 1
        fi
    else
        log_message "==================== BACKUP FAILED ===================="
        send_notification "failed" "Backup creation failed"
        exit 1
    fi
}

main "$@"
