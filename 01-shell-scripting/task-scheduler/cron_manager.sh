#!/bin/bash

# Script quản lý cron jobs
# Cho phép thêm, xóa, liệt kê và kiểm tra cron jobs

set -e

# Màu sắc cho output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Hàm hiển thị usage
show_usage() {
    cat << EOF
Sử dụng: $0 [OPTIONS]

Quản lý Cron Jobs

OPTIONS:
    -l, --list              Liệt kê tất cả cron jobs hiện tại
    -a, --add               Thêm cron job mới
    -d, --delete <id>       Xóa cron job theo ID
    -e, --edit              Chỉnh sửa crontab
    -h, --help              Hiển thị trợ giúp

VÍ DỤ:
    $0 --list
    $0 --add
    $0 --delete 3
    $0 --edit

CRON FORMAT:
    * * * * * command
    │ │ │ │ │
    │ │ │ │ └─── Ngày trong tuần (0-7, 0 và 7 là Chủ nhật)
    │ │ │ └───── Tháng (1-12)
    │ │ └─────── Ngày trong tháng (1-31)
    │ └───────── Giờ (0-23)
    └─────────── Phút (0-59)
EOF
}

# Hàm liệt kê cron jobs
list_cron_jobs() {
    echo -e "${GREEN}=== Danh sách Cron Jobs ===${NC}"
    echo ""
    
    if crontab -l 2>/dev/null | grep -v "^#" | grep -v "^$" > /dev/null; then
        local count=1
        crontab -l 2>/dev/null | grep -v "^#" | grep -v "^$" | while read -r line; do
            echo -e "${YELLOW}[$count]${NC} $line"
            count=$((count + 1))
        done
    else
        echo -e "${YELLOW}Không có cron job nào.${NC}"
    fi
    echo ""
}

# Hàm thêm cron job mới
add_cron_job() {
    echo -e "${GREEN}=== Thêm Cron Job Mới ===${NC}"
    echo ""
    
    # Hiển thị các ví dụ
    cat << EOF
Ví dụ cron expressions:
  0 2 * * *       - Chạy lúc 2:00 sáng mỗi ngày
  */15 * * * *    - Chạy mỗi 15 phút
  0 9-17 * * 1-5  - Chạy mỗi giờ từ 9AM-5PM, Thứ Hai đến Thứ Sáu
  0 0 1 * *       - Chạy vào ngày đầu tiên của mỗi tháng

EOF
    
    # Nhập thông tin
    read -p "Nhập cron expression (ví dụ: 0 2 * * *): " cron_time
    read -p "Nhập lệnh cần thực thi: " command
    read -p "Nhập mô tả (tùy chọn): " description
    
    # Xác nhận
    echo ""
    echo -e "${YELLOW}Cron job sẽ được thêm:${NC}"
    echo "  Thời gian: $cron_time"
    echo "  Lệnh: $command"
    [ -n "$description" ] && echo "  Mô tả: $description"
    echo ""
    
    read -p "Xác nhận thêm? (y/n): " confirm
    
    if [[ "$confirm" == "y" || "$confirm" == "Y" ]]; then
        # Backup crontab hiện tại
        crontab -l > /tmp/crontab.bak 2>/dev/null || true
        
        # Thêm job mới
        {
            crontab -l 2>/dev/null || true
            [ -n "$description" ] && echo "# $description"
            echo "$cron_time $command"
        } | crontab -
        
        echo -e "${GREEN}Đã thêm cron job thành công!${NC}"
    else
        echo -e "${YELLOW}Đã hủy.${NC}"
    fi
}

# Hàm xóa cron job
delete_cron_job() {
    local job_id=$1
    
    if [ -z "$job_id" ]; then
        echo -e "${RED}Lỗi: Vui lòng cung cấp ID của job cần xóa.${NC}"
        return 1
    fi
    
    echo -e "${GREEN}=== Xóa Cron Job ===${NC}"
    echo ""
    
    # Hiển thị danh sách
    list_cron_jobs
    
    # Lấy job theo ID
    local job_line=$(crontab -l 2>/dev/null | grep -v "^#" | grep -v "^$" | sed -n "${job_id}p")
    
    if [ -z "$job_line" ]; then
        echo -e "${RED}Lỗi: Không tìm thấy job với ID $job_id${NC}"
        return 1
    fi
    
    echo -e "${YELLOW}Job cần xóa:${NC}"
    echo "  $job_line"
    echo ""
    
    read -p "Xác nhận xóa? (y/n): " confirm
    
    if [[ "$confirm" == "y" || "$confirm" == "Y" ]]; then
        # Backup
        crontab -l > /tmp/crontab.bak 2>/dev/null || true
        
        # Xóa job
        crontab -l 2>/dev/null | grep -v "^#" | grep -v "^$" | sed "${job_id}d" | crontab -
        
        echo -e "${GREEN}Đã xóa cron job thành công!${NC}"
    else
        echo -e "${YELLOW}Đã hủy.${NC}"
    fi
}

# Hàm chỉnh sửa crontab trực tiếp
edit_crontab() {
    echo -e "${GREEN}=== Chỉnh sửa Crontab ===${NC}"
    echo ""
    
    # Backup
    crontab -l > /tmp/crontab.bak 2>/dev/null || true
    echo -e "${YELLOW}Đã backup crontab vào /tmp/crontab.bak${NC}"
    
    # Mở editor
    crontab -e
}

# Main
if [ $# -eq 0 ]; then
    show_usage
    exit 0
fi

case "$1" in
    -l|--list)
        list_cron_jobs
        ;;
    -a|--add)
        add_cron_job
        ;;
    -d|--delete)
        delete_cron_job "$2"
        ;;
    -e|--edit)
        edit_crontab
        ;;
    -h|--help)
        show_usage
        ;;
    *)
        echo -e "${RED}Lỗi: Tùy chọn không hợp lệ: $1${NC}"
        echo ""
        show_usage
        exit 1
        ;;
esac
