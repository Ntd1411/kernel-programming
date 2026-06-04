# Phần 1: Lập Trình Shell

Các script bash để tự động hóa quản lý hệ thống Ubuntu/Linux.

## Mục Tiêu

- Quản lý file và thư mục tự động
- Lập lịch tác vụ định kỳ
- Thiết lập và đồng bộ thời gian hệ thống
- Cài đặt/gỡ bỏ phần mềm tự động

## Cấu Trúc

### 1. File Management (file-management/)

Script quản lý file:

- `file_manager.sh` - Script chính quản lý file
- `backup.sh` - Backup tự động
- `find_duplicates.sh` - Tìm file trùng lặp
- `cleanup.sh` - Dọn dẹp file tạm

### 2. Task Scheduler (task-scheduler/)

Lập lịch tác vụ:

- `cron_manager.sh` - Quản lý cron jobs
- `scheduled_tasks.sh` - Các tác vụ định kỳ
- `systemd_timer/` - Systemd timer units
  - `backup.service` - Service cho backup tự động
  - `backup.timer` - Timer cho backup
  - `cleanup.service` - Service cho cleanup
  - `cleanup.timer` - Timer cho cleanup
  - `README.md` - Hướng dẫn sử dụng systemd timer

### 3. Time Management (time-management/)

Quản lý thời gian:

- `time_tracker.sh` - Theo dõi thời gian làm việc
- `stopwatch.sh` - Đồng hồ bấm giờ

### 4. Package Management (package-management/)

Quản lý package:

- `package_manager.sh` - Quản lý cài đặt/gỡ bỏ package
- `dependency_checker.sh` - Kiểm tra dependencies
- `repo_manager.sh` - Quản lý repository

## Cách Sử Dụng

### Cấp quyền thực thi

```bash
chmod +x script_name.sh
```

## Hướng Dẫn Chi Tiết

### 1. File Management

#### file_manager.sh - Menu quản lý file tương tác

```bash
./file-management/file_manager.sh
```

Cung cấp menu tương tác với các chức năng:
- Tìm kiếm file theo pattern
- Sao chép/di chuyển file
- Xóa file/thư mục
- Nén và giải nén
- Thay đổi quyền truy cập
- Hiển thị thông tin file
- Tìm file lớn nhất

#### backup.sh - Backup tự động

```bash
./file-management/backup.sh <source_dir> <backup_dir> [retention_days]
```

Ví dụ:
```bash
# Backup /home/user vào /backup, giữ 7 ngày
./file-management/backup.sh /home/user /backup 7

# Backup với retention mặc định (7 ngày)
./file-management/backup.sh /var/www /backup
```

Tính năng:
- Tạo file nén .tar.gz với timestamp
- Tự động xóa backup cũ theo số ngày retention
- Ghi log chi tiết
- Kiểm tra dependencies và quyền truy cập

#### find_duplicates.sh - Tìm file trùng lặp

```bash
./file-management/find_duplicates.sh <directory> [action]
```

Action: `list` (mặc định), `delete`, `move`

Ví dụ:
```bash
# Liệt kê file trùng lặp
./file-management/find_duplicates.sh /home/user/Documents

# Xóa file trùng lặp (giữ 1 bản)
./file-management/find_duplicates.sh /home/user/Downloads delete

# Di chuyển file trùng lặp vào thư mục khác
./file-management/find_duplicates.sh /home/user/Pictures move
```

Sử dụng MD5 hash để phát hiện file giống hệt nhau.

#### cleanup.sh - Dọn dẹp file tạm

```bash
./file-management/cleanup.sh [options]
```

Options:
- `-d, --days DAYS` - Xóa file cũ hơn DAYS ngày (mặc định: 30)
- `-n, --dry-run` - Chỉ hiển thị, không xóa
- `-v, --verbose` - Hiển thị chi tiết

Ví dụ:
```bash
# Xem file cũ hơn 7 ngày
./file-management/cleanup.sh -d 7 -n

# Xóa file cũ hơn 30 ngày với verbose
./file-management/cleanup.sh -d 30 -v
```

### 2. Task Scheduler

#### cron_manager.sh - Quản lý cron jobs

```bash
./task-scheduler/cron_manager.sh [OPTIONS]
```

Options:
- `-l, --list` - Liệt kê tất cả cron jobs
- `-a, --add` - Thêm cron job mới
- `-d, --delete <id>` - Xóa cron job theo ID
- `-e, --edit` - Chỉnh sửa crontab

Ví dụ:
```bash
# Liệt kê cron jobs
./task-scheduler/cron_manager.sh --list

# Thêm cron job mới
./task-scheduler/cron_manager.sh --add

# Xóa job số 3
./task-scheduler/cron_manager.sh --delete 3
```

Cron format: `phút giờ ngày tháng ngày_trong_tuần lệnh`

#### scheduled_tasks.sh - Tác vụ định kỳ

```bash
./task-scheduler/scheduled_tasks.sh [task_name]
```

Các tác vụ có sẵn:
- `daily_backup` - Backup hàng ngày
- `cleanup_temp_files` - Dọn dẹp file tạm
- `system_update` - Cập nhật hệ thống
- `log_rotation` - Rotate log files

Ví dụ:
```bash
# Chạy backup hàng ngày
./task-scheduler/scheduled_tasks.sh daily_backup

# Dọn dẹp file tạm
./task-scheduler/scheduled_tasks.sh cleanup_temp_files
```

#### Systemd Timer

Xem hướng dẫn chi tiết trong `task-scheduler/systemd_timer/README.md`

Cài đặt timer:
```bash
cd task-scheduler/systemd_timer
sudo cp *.service *.timer /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable backup.timer
sudo systemctl start backup.timer
```

Kiểm tra timer:
```bash
systemctl list-timers
systemctl status backup.timer
journalctl -u backup.service
```

### 3. Time Management

#### time_tracker.sh - Theo dõi thời gian

```bash
./time-management/time_tracker.sh [option]
```

Tính năng:
- Hiển thị thời gian hiện tại (local, UTC, timestamp)
- Xem uptime hệ thống
- Hiển thị múi giờ thế giới
- Theo dõi thời gian boot

Ví dụ:
```bash
# Hiển thị thông tin thời gian
./time-management/time_tracker.sh
```

#### stopwatch.sh - Đồng hồ bấm giờ

```bash
./time-management/stopwatch.sh [command] [name]
```

Commands:
- `start [name]` - Bắt đầu đồng hồ
- `stop` - Dừng và hiển thị thời gian
- `status` - Xem trạng thái hiện tại

Ví dụ:
```bash
# Bắt đầu bấm giờ
./time-management/stopwatch.sh start "coding-session"

# Dừng và xem kết quả
./time-management/stopwatch.sh stop

# Kiểm tra trạng thái
./time-management/stopwatch.sh status
```

### 4. Package Management

#### package_manager.sh - Quản lý package

```bash
./package-management/package_manager.sh [command] [package_name]
```

Commands:
- `install <package>` - Cài đặt package
- `remove <package>` - Gỡ bỏ package
- `search <keyword>` - Tìm kiếm package
- `update` - Cập nhật danh sách package
- `upgrade` - Nâng cấp tất cả package
- `list` - Liệt kê package đã cài

Ví dụ:
```bash
# Cài đặt package
sudo ./package-management/package_manager.sh install vim

# Tìm kiếm package
./package-management/package_manager.sh search python

# Cập nhật hệ thống
sudo ./package-management/package_manager.sh update
sudo ./package-management/package_manager.sh upgrade
```

Hỗ trợ nhiều package manager: apt, dnf, yum, pacman, zypper

#### dependency_checker.sh - Kiểm tra dependencies

```bash
./package-management/dependency_checker.sh <package_name>
```

Ví dụ:
```bash
# Kiểm tra dependencies của vim
./package-management/dependency_checker.sh vim

# Xem package nào phụ thuộc vào libssl
./package-management/dependency_checker.sh libssl-dev
```

Hiển thị:
- Dependencies trực tiếp
- Reverse dependencies (package phụ thuộc vào nó)

#### repo_manager.sh - Quản lý repository

```bash
./package-management/repo_manager.sh [command]
```

Commands:
- `list` - Liệt kê tất cả repository
- `add <repo>` - Thêm repository mới
- `remove <repo>` - Xóa repository
- `update` - Cập nhật repository

Ví dụ:
```bash
# Liệt kê repositories
./package-management/repo_manager.sh list

# Thêm PPA (Ubuntu)
sudo ./package-management/repo_manager.sh add ppa:user/repo

# Cập nhật repository cache
sudo ./package-management/repo_manager.sh update
```

## Kiến Thức Cần Thiết

### Bash Basics

- Variables và parameter expansion
- Conditional statements (if/else/case)
- Loops (for/while/until)
- Functions
- Command-line arguments

### File Operations

- `ls`, `cd`, `mkdir`, `rm`, `cp`, `mv`
- `find`, `grep`, `sed`, `awk`
- `tar`, `gzip`, `zip`
- File permissions: `chmod`, `chown`

### Cron và Scheduling

- Crontab syntax: `* * * * * command`
- `crontab -e`, `crontab -l`
- Systemd timers

### Package Management

- `apt`, `apt-get`, `dpkg`
- `apt-cache search`
- Dependency resolution
- Repository management

## Bài Tập

1. Cấu hình systemd timer để backup tự động mỗi ngày
2. Viết script tìm và xóa file trùng lặp
3. Tạo time tracker để theo dõi thời gian làm việc các dự án
4. Viết script kiểm tra và cài đặt missing dependencies
5. Tạo cron job để dọn dẹp file tạm định kỳ

## Lưu Ý

- Luôn kiểm tra quyền truy cập trước khi thao tác file
- Sử dụng `set -e` để script dừng khi có lỗi
- Log tất cả hoạt động quan trọng
- Test script trên môi trường an toàn trước khi deploy
- Không hardcode password trong script
