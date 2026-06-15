#!/bin/bash

# find_duplicates.sh - Tìm file trùng lặp dựa trên MD5 hash
# Sử dụng: ./find_duplicates.sh <directory> [action]
# action: list (mặc định), delete, move

set -e

if [ $# -lt 1 ]; then
    echo "Sử dụng: $0 <directory> [list|delete|move]"
    echo "Ví dụ: $0 /home/user/Documents list"
    exit 1
fi

SEARCH_DIR="$1"
ACTION="${2:-list}"
TEMP_FILE="/tmp/duplicates_$$.txt"
LOG_FILE="duplicates_$(date +%Y%m%d_%H%M%S).log"

cleanup() {
    rm -f "$TEMP_FILE"
}

trap cleanup EXIT

log_message() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

if [ ! -d "$SEARCH_DIR" ]; then
    echo "Lỗi: Thư mục không tồn tại: $SEARCH_DIR"
    exit 1
fi

echo "Tìm kiếm file trùng lặp trong: $SEARCH_DIR"
echo "Đang tính MD5 hash..."

find "$SEARCH_DIR" -type f -print0 | while IFS= read -r -d '' file; do
    if [ -r "$file" ]; then
        hash=$(md5sum "$file" 2>/dev/null | cut -d' ' -f1)
        size=$(stat -c%s "$file")
        echo "$hash|$size|$file"
    fi
done > "$TEMP_FILE"

total_files=$(wc -l < "$TEMP_FILE")
echo "Đã quét $total_files files"

echo ""
echo "Phân tích kết quả..."

duplicates_found=0
total_wasted_space=0

while IFS='|' read -r hash size file_list; do
    duplicates_found=$((duplicates_found + 1))
    
    IFS=',' read -ra FILES <<< "$file_list"
    num_duplicates=${#FILES[@]}
    
    wasted_space=$((size * (num_duplicates - 1)))
    total_wasted_space=$((total_wasted_space + wasted_space))
    
    echo "===================="
    log_message "Nhóm trùng lặp #$duplicates_found"
    log_message "Hash: $hash"
    log_message "Kích thước: $(numfmt --to=iec-i --suffix=B $size)"
    log_message "Số file trùng: $num_duplicates"
    log_message "Dung lượng lãng phí: $(numfmt --to=iec-i --suffix=B $wasted_space)"
    echo ""
    
    for i in "${!FILES[@]}"; do
        file="${FILES[$i]}"
        if [ $i -eq 0 ]; then
            log_message "  [ORIGINAL] $file"
        else
            log_message "  [DUPLICATE] $file"
            
            case "$ACTION" in
                delete)
                    echo -n "Xóa file trùng lặp? (y/n): "
                    read -r confirm
                    if [ "$confirm" = "y" ]; then
                        rm -f "$file"
                        log_message "    -> ĐÃ XÓA"
                    else
                        log_message "    -> BỎ QUA"
                    fi
                    ;;
                move)
                    DUPLICATE_DIR="${SEARCH_DIR}/duplicates"
                    mkdir -p "$DUPLICATE_DIR"
                    new_name="${DUPLICATE_DIR}/$(basename "$file")"
                    mv "$file" "$new_name"
                    log_message "    -> DI CHUYỂN đến $new_name"
                    ;;
                list)
                    ;;
            esac
        fi
    done
    
    echo ""
done < <(sort "$TEMP_FILE" | \
awk -F'|' '{
    key = $1 "|" $2
    files[key] = files[key] ? files[key] "," $3 : $3
    count[key]++
    size[key] = $2
}
END {
    for (k in files) {
        if (count[k] > 1) {
            print k "|" files[k]
        }
    }
}')

if [ $duplicates_found -eq 0 ]; then
    echo "Không tìm thấy file trùng lặp!"
else
    echo "===================="
    echo "TỔNG KẾT:"
    echo "Số nhóm trùng lặp: $duplicates_found"
    echo "Tổng dung lượng lãng phí: $(numfmt --to=iec-i --suffix=B $total_wasted_space)"
    echo "Log file: $LOG_FILE"
fi
