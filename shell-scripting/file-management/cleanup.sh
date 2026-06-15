#!/bin/bash

# cleanup.sh - Dọn dẹp file tạm và file cũ
# Sử dụng: ./cleanup.sh [options]

set -e

DAYS_OLD=30
DRY_RUN=0
VERBOSE=0

show_usage() {
    cat << EOF
Sử dụng: $0 [options]

Options:
    -d, --days DAYS        Xóa file cũ hơn DAYS ngày (mặc định: 30)
    -n, --dry-run          Chỉ hiển thị, không xóa thực sự
    -v, --verbose          Hiển thị chi tiết
    -h, --help             Hiển thị trợ giúp

Ví dụ:
    $0 -d 7 -n             Xem file cũ hơn 7 ngày
    $0 -d 30 -v            Xóa file cũ hơn 30 ngày (verbose)
EOF
}

while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--days)
            DAYS_OLD="$2"
            shift 2
            ;;
        -n|--dry-run)
            DRY_RUN=1
            shift
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        *)
            echo "Tùy chọn không hợp lệ: $1"
            show_usage
            exit 1
            ;;
    esac
done

LOG_FILE="cleanup_$(date +%Y%m%d_%H%M%S).log"

log_message() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

[ $VERBOSE -eq 1 ] && log_message "==================== CLEANUP START ===================="
[ $VERBOSE -eq 1 ] && log_message "Xóa file cũ hơn: $DAYS_OLD ngày"
[ $DRY_RUN -eq 1 ] && log_message "CHẾ ĐỘ DRY-RUN (không xóa thực sự)"

TEMP_DIRS=(
    "/tmp"
    "/var/tmp"
    "$HOME/.cache"
)

TEMP_PATTERNS=(
    "*.tmp"
    "*.temp"
    "*~"
    "*.bak"
    "*.old"
    "*.swp"
    "core"
)

LOG_DIRS=(
    "/var/log"
    "$HOME/.local/share/logs"
)

total_size=0
total_files=0

cleanup_temp_dirs() {
    log_message "Dọn dẹp thư mục tạm..."
    
    for dir in "${TEMP_DIRS[@]}"; do
        if [ -d "$dir" ]; then
            [ $VERBOSE -eq 1 ] && log_message "Kiểm tra: $dir"
            
            for pattern in "${TEMP_PATTERNS[@]}"; do
                while IFS= read -r -d '' file; do
                    size=$(stat -c%s "$file" 2>/dev/null || echo 0)
                    total_size=$((total_size + size))
                    total_files=$((total_files + 1))
                    
                    if [ $DRY_RUN -eq 1 ]; then
                        log_message "[DRY-RUN] Sẽ xóa: $file ($(numfmt --to=iec-i --suffix=B $size))"
                    else
                        if [ $VERBOSE -eq 1 ]; then
                            log_message "Xóa: $file ($(numfmt --to=iec-i --suffix=B $size))"
                        fi
                        rm -f "$file"
                    fi
                done < <(find "$dir" -maxdepth 2 -name "$pattern" -type f -mtime +$DAYS_OLD -print0 2>/dev/null)
            done
        fi
    done
}

cleanup_logs() {
    log_message "Dọn dẹp log files cũ..."
    
    for dir in "${LOG_DIRS[@]}"; do
        if [ -d "$dir" ] && [ -r "$dir" ]; then
            [ $VERBOSE -eq 1 ] && log_message "Kiểm tra: $dir"
            
            while IFS= read -r -d '' file; do
                size=$(stat -c%s "$file" 2>/dev/null || echo 0)
                total_size=$((total_size + size))
                total_files=$((total_files + 1))
                
                if [ $DRY_RUN -eq 1 ]; then
                    log_message "[DRY-RUN] Sẽ xóa: $file ($(numfmt --to=iec-i --suffix=B $size))"
                else
                    if [ $VERBOSE -eq 1 ]; then
                        log_message "Xóa: $file ($(numfmt --to=iec-i --suffix=B $size))"
                    fi
                    rm -f "$file"
                fi
            done < <(find "$dir" -name "*.log.*" -o -name "*.gz" -type f -mtime +$DAYS_OLD -print0 2>/dev/null)
        fi
    done
}

cleanup_package_cache() {
    log_message "Dọn dẹp package cache..."
    
    if command -v apt-get &> /dev/null; then
        if [ $DRY_RUN -eq 1 ]; then
            log_message "[DRY-RUN] Sẽ chạy: apt-get autoclean"
        else
            [ $VERBOSE -eq 1 ] && log_message "Chạy: apt-get autoclean"
            sudo apt-get autoclean -y >> "$LOG_FILE" 2>&1
        fi
    fi
}

cleanup_thumbnails() {
    log_message "Dọn dẹp thumbnails cũ..."
    
    THUMB_DIR="$HOME/.cache/thumbnails"
    
    if [ -d "$THUMB_DIR" ]; then
        while IFS= read -r -d '' file; do
            size=$(stat -c%s "$file" 2>/dev/null || echo 0)
            total_size=$((total_size + size))
            total_files=$((total_files + 1))
            
            if [ $DRY_RUN -eq 1 ]; then
                [ $VERBOSE -eq 1 ] && log_message "[DRY-RUN] Sẽ xóa: $file"
            else
                rm -f "$file"
            fi
        done < <(find "$THUMB_DIR" -type f -mtime +$DAYS_OLD -print0 2>/dev/null)
    fi
}

cleanup_browser_cache() {
    log_message "Dọn dẹp browser cache..."
    
    BROWSER_CACHES=(
        "$HOME/.cache/mozilla"
        "$HOME/.cache/google-chrome"
        "$HOME/.cache/chromium"
    )
    
    for cache_dir in "${BROWSER_CACHES[@]}"; do
        if [ -d "$cache_dir" ]; then
            cache_size=$(du -sb "$cache_dir" 2>/dev/null | cut -f1)
            
            if [ $DRY_RUN -eq 1 ]; then
                log_message "[DRY-RUN] Sẽ xóa cache: $cache_dir ($(numfmt --to=iec-i --suffix=B $cache_size))"
            else
                [ $VERBOSE -eq 1 ] && log_message "Xóa cache: $cache_dir"
                rm -rf "$cache_dir"/*
                total_size=$((total_size + cache_size))
            fi
        fi
    done
}

main() {
    cleanup_temp_dirs
    cleanup_logs
    cleanup_thumbnails
    
    if [ "$EUID" -eq 0 ]; then
        cleanup_package_cache
    fi
    
    log_message "==================== KẾT QUẢ ===================="
    log_message "Tổng số file: $total_files"
    log_message "Dung lượng giải phóng: $(numfmt --to=iec-i --suffix=B $total_size)"
    
    if [ $DRY_RUN -eq 1 ]; then
        log_message "CHẾ ĐỘ DRY-RUN: Không có file nào bị xóa thực sự"
    fi
    
    log_message "Log file: $LOG_FILE"
    log_message "==================== CLEANUP HOÀN TẤT ===================="
}

main "$@"
