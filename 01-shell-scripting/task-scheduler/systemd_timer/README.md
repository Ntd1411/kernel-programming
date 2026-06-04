# Systemd Timer Units

Systemd timer là cách hiện đại để lập lịch tác vụ trên Linux, thay thế cho cron.

## Cài Đặt

### 1. Copy các file service và timer

```bash
sudo cp *.service /etc/systemd/system/
sudo cp *.timer /etc/systemd/system/
```

### 2. Reload systemd

```bash
sudo systemctl daemon-reload
```

### 3. Kích hoạt và khởi động timer

```bash
# Kích hoạt timer backup
sudo systemctl enable backup.timer
sudo systemctl start backup.timer

# Kích hoạt timer cleanup
sudo systemctl enable cleanup.timer
sudo systemctl start cleanup.timer
```

## Quản Lý Timer

### Kiểm tra trạng thái

```bash
# Liệt kê tất cả timers
systemctl list-timers

# Kiểm tra trạng thái timer cụ thể
systemctl status backup.timer
systemctl status cleanup.timer
```

### Xem log

```bash
# Xem log của service
journalctl -u backup.service
journalctl -u cleanup.service

# Xem log theo thời gian thực
journalctl -u backup.service -f
```

### Chạy thủ công

```bash
# Chạy service ngay lập tức
sudo systemctl start backup.service
sudo systemctl start cleanup.service
```

### Dừng và vô hiệu hóa

```bash
# Dừng timer
sudo systemctl stop backup.timer
sudo systemctl stop cleanup.timer

# Vô hiệu hóa (không khởi động cùng hệ thống)
sudo systemctl disable backup.timer
sudo systemctl disable cleanup.timer
```

## Cấu Trúc File

### Service File (.service)

Định nghĩa tác vụ cần chạy:

```ini
[Unit]
Description=Mô tả dịch vụ
After=network.target

[Service]
Type=oneshot
ExecStart=/path/to/script.sh
User=root
```

### Timer File (.timer)

Định nghĩa lịch trình:

```ini
[Unit]
Description=Mô tả timer
Requires=my-task.service

[Timer]
OnCalendar=*-*-* 02:00:00
Persistent=true

[Install]
WantedBy=timers.target
```

## OnCalendar Syntax

Format: `DayOfWeek Year-Month-Day Hour:Minute:Second`

Ví dụ:
- `*-*-* 02:00:00` - Hàng ngày lúc 2:00 sáng
- `Mon *-*-* 09:00:00` - Mỗi thứ Hai lúc 9:00 sáng
- `*-*-01 00:00:00` - Ngày đầu tiên của mỗi tháng
- `*-*-* *:00/15:00` - Mỗi 15 phút
- `Sun *-*-* 03:00:00` - Mỗi Chủ nhật lúc 3:00 sáng

## Ưu Điểm của Systemd Timer

1. **Tích hợp tốt**: Tích hợp với systemd và journald
2. **Logging**: Log tập trung qua journalctl
3. **Dependency**: Có thể định nghĩa dependencies
4. **Persistent**: Chạy task bị miss nếu hệ thống tắt
5. **Flexible**: Syntax linh hoạt hơn cron
6. **Monitoring**: Dễ dàng monitor qua systemctl

## So Sánh với Cron

| Tính năng | Cron | Systemd Timer |
|-----------|------|---------------|
| Syntax | Khó nhớ | Dễ đọc |
| Logging | Phân tán | Tập trung (journald) |
| Dependencies | Không | Có |
| Missed runs | Bỏ qua | Có thể chạy lại |
| Monitor | Khó | Dễ (systemctl) |
