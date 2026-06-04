#!/bin/bash

# file_manager.sh - Script quản lý file tổng hợp
# Sử dụng: ./file_manager.sh [option]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOG_FILE="${SCRIPT_DIR}/file_manager.log"

log_message() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

show_menu() {
    echo "================================"
    echo "     FILE MANAGER MENU"
    echo "================================"
    echo "1. Tìm kiếm file"
    echo "2. Sao chép file/thư mục"
    echo "3. Di chuyển file/thư mục"
    echo "4. Xóa file/thư mục"
    echo "5. Nén file/thư mục"
    echo "6. Giải nén file"
    echo "7. Thay đổi quyền"
    echo "8. Hiển thị thông tin file"
    echo "9. Tìm file lớn nhất"
    echo "0. Thoát"
    echo "================================"
}

search_files() {
    echo "Nhập đường dẫn tìm kiếm:"
    read -r search_path
    
    echo "Nhập tên file/pattern (vd: *.txt):"
    read -r pattern
    
    log_message "Tìm kiếm: $pattern trong $search_path"
    
    if [ -d "$search_path" ]; then
        echo "Kết quả tìm kiếm:"
        find "$search_path" -name "$pattern" -type f 2>/dev/null | while read -r file; do
            size=$(du -h "$file" | cut -f1)
            echo "  $file (Size: $size)"
        done
    else
        echo "Lỗi: Đường dẫn không tồn tại!"
    fi
}

copy_files() {
    echo "Nhập đường dẫn nguồn:"
    read -r source
    
    echo "Nhập đường dẫn đích:"
    read -r dest
    
    if [ -e "$source" ]; then
        if cp -r "$source" "$dest"; then
            log_message "Sao chép thành công: $source -> $dest"
            echo "Sao chép thành công!"
        else
            log_message "Lỗi sao chép: $source -> $dest"
            echo "Lỗi: Không thể sao chép!"
        fi
    else
        echo "Lỗi: Nguồn không tồn tại!"
    fi
}

move_files() {
    echo "Nhập đường dẫn nguồn:"
    read -r source
    
    echo "Nhập đường dẫn đích:"
    read -r dest
    
    if [ -e "$source" ]; then
        if mv "$source" "$dest"; then
            log_message "Di chuyển thành công: $source -> $dest"
            echo "Di chuyển thành công!"
        else
            log_message "Lỗi di chuyển: $source -> $dest"
            echo "Lỗi: Không thể di chuyển!"
        fi
    else
        echo "Lỗi: Nguồn không tồn tại!"
    fi
}

delete_files() {
    echo "Nhập đường dẫn cần xóa:"
    read -r target
    
    if [ -e "$target" ]; then
        echo "Bạn có chắc muốn xóa '$target'? (y/n)"
        read -r confirm
        
        if [ "$confirm" = "y" ] || [ "$confirm" = "Y" ]; then
            if rm -rf "$target"; then
                log_message "Đã xóa: $target"
                echo "Xóa thành công!"
            else
                log_message "Lỗi xóa: $target"
                echo "Lỗi: Không thể xóa!"
            fi
        else
            echo "Đã hủy xóa."
        fi
    else
        echo "Lỗi: Đường dẫn không tồn tại!"
    fi
}

compress_files() {
    echo "Nhập đường dẫn cần nén:"
    read -r source
    
    echo "Nhập tên file đầu ra (vd: backup.tar.gz):"
    read -r output
    
    if [ -e "$source" ]; then
        if tar -czf "$output" "$source"; then
            log_message "Nén thành công: $source -> $output"
            echo "Nén thành công! File: $output"
            ls -lh "$output"
        else
            log_message "Lỗi nén: $source"
            echo "Lỗi: Không thể nén!"
        fi
    else
        echo "Lỗi: Nguồn không tồn tại!"
    fi
}

extract_files() {
    echo "Nhập đường dẫn file nén:"
    read -r archive
    
    echo "Nhập thư mục đích (để trống = thư mục hiện tại):"
    read -r dest
    
    [ -z "$dest" ] && dest="."
    
    if [ -f "$archive" ]; then
        case "$archive" in
            *.tar.gz|*.tgz)
                tar -xzf "$archive" -C "$dest"
                ;;
            *.tar.bz2|*.tbz2)
                tar -xjf "$archive" -C "$dest"
                ;;
            *.tar)
                tar -xf "$archive" -C "$dest"
                ;;
            *.zip)
                unzip "$archive" -d "$dest"
                ;;
            *.gz)
                gunzip "$archive"
                ;;
            *)
                echo "Lỗi: Định dạng không được hỗ trợ!"
                return 1
                ;;
        esac
        
        log_message "Giải nén thành công: $archive -> $dest"
        echo "Giải nén thành công!"
    else
        echo "Lỗi: File không tồn tại!"
    fi
}

change_permissions() {
    echo "Nhập đường dẫn:"
    read -r target
    
    echo "Nhập quyền (vd: 755, 644):"
    read -r perms
    
    if [ -e "$target" ]; then
        if chmod "$perms" "$target"; then
            log_message "Đổi quyền: $target -> $perms"
            echo "Đổi quyền thành công!"
            ls -l "$target"
        else
            echo "Lỗi: Không thể đổi quyền!"
        fi
    else
        echo "Lỗi: Đường dẫn không tồn tại!"
    fi
}

show_file_info() {
    echo "Nhập đường dẫn file:"
    read -r target
    
    if [ -e "$target" ]; then
        echo "Thông tin file: $target"
        echo "================================"
        ls -lh "$target"
        echo ""
        file "$target"
        echo ""
        echo "MD5: $(md5sum "$target" 2>/dev/null | cut -d' ' -f1)"
        echo "Quyền: $(stat -c '%a' "$target")"
        echo "Owner: $(stat -c '%U:%G' "$target")"
        echo "Last modified: $(stat -c '%y' "$target")"
    else
        echo "Lỗi: File không tồn tại!"
    fi
}

find_largest_files() {
    echo "Nhập đường dẫn tìm kiếm:"
    read -r search_path
    
    echo "Số lượng file hiển thị (mặc định: 10):"
    read -r count
    
    [ -z "$count" ] && count=10
    
    if [ -d "$search_path" ]; then
        echo "Top $count file lớn nhất trong $search_path:"
        echo "================================"
        du -ah "$search_path" 2>/dev/null | sort -rh | head -n "$count"
    else
        echo "Lỗi: Đường dẫn không tồn tại!"
    fi
}

main() {
    log_message "File Manager khởi động"
    
    while true; do
        echo ""
        show_menu
        echo -n "Chọn chức năng: "
        read -r choice
        
        case $choice in
            1) search_files ;;
            2) copy_files ;;
            3) move_files ;;
            4) delete_files ;;
            5) compress_files ;;
            6) extract_files ;;
            7) change_permissions ;;
            8) show_file_info ;;
            9) find_largest_files ;;
            0)
                log_message "File Manager thoát"
                echo "Tạm biệt!"
                exit 0
                ;;
            *)
                echo "Lựa chọn không hợp lệ!"
                ;;
        esac
    done
}

main "$@"
