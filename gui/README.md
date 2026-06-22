# Kernel Linux GUI - Ứng Dụng Quản Lý Dự Án

Ứng dụng GUI bằng Python để quản lý và chạy các phần trong dự án Kernel Linux một cách dễ dàng.

## Tính Năng

- Giao diện đồ họa thân thiện với 3 tabs chính:
  - **Shell Scripting**: Quản lý các script bash
  - **System Programming**: Compile và chạy các chương trình C system
  - **SMP Programming**: Compile và chạy các ví dụ về đồng bộ hóa

- Terminal realtime tích hợp:
  - Hiển thị output realtime từ các lệnh
  - Hỗ trợ tương tác với process đang chạy
  - Gửi input đến process
  - Dừng process đang chạy
  - Xóa terminal

- Các chức năng nhanh:
  - Compile từng module riêng lẻ
  - Compile tất cả modules
  - Clean tất cả modules
  - Xem file README
  - Chạy demo scripts

## Yêu Cầu Hệ Thống

- Python 3.6 trở lên
- Tkinter (thường đã có sẵn trong Python)
- Linux/Ubuntu (khuyến nghị) hoặc Windows với WSL

### Cài Đặt Trên Ubuntu/Linux

```bash
# Cài đặt Python3 và Tkinter
sudo apt update
sudo apt install python3 python3-tk

# Cấp quyền thực thi cho script
chmod +x run_gui.sh

# Chạy ứng dụng
./run_gui.sh
```

### Cài Đặt Trên Windows

```batch
# Tải Python từ https://www.python.org/downloads/
# Tkinter thường đã có sẵn trong Python Windows

# Chạy ứng dụng
run_gui.bat
```

hoặc:

```batch
python app.py
```

## Cách Sử Dụng

### 1. Tab Shell Scripting

- Chọn module muốn chạy (File Management, Task Scheduler, etc.)
- Click vào nút tương ứng với script muốn chạy
- Xem output trong terminal phía dưới
- Click "README" để xem hướng dẫn chi tiết

### 2. Tab System Programming

- Click "Compile" để build module
- Click "Chạy [tên]" để chạy executable đã compile
- Click "Test" để chạy test scripts
- Sử dụng "Compile Tất Cả" để build tất cả modules
- Sử dụng "Clean Tất Cả" để xóa các file build

### 3. Tab SMP Programming

- Mỗi ví dụ có 3 nút:
  - **Compile**: Build ví dụ
  - **Chạy**: Thực thi chương trình
  - **README**: Xem tài liệu chi tiết
- Sử dụng "Compile Tất Cả" để build tất cả ví dụ
- Click "Chạy Demo Tổng Hợp" để chạy tất cả demo

### 4. Terminal Realtime

- Output hiển thị realtime khi chạy lệnh
- Nhập input vào ô "Input" và nhấn Enter hoặc click "Gửi"
- Click "Dừng Process" để terminate process đang chạy
- Click "Xóa Terminal" để xóa output cũ

## Cấu Trúc Thư Mục

```
gui/
├── app.py              # File chính ứng dụng
├── run_gui.sh          # Script chạy trên Linux
├── run_gui.bat         # Script chạy trên Windows
├── requirements.txt    # Dependencies (không cần cài thêm)
└── README.md          # File này
```

## Troubleshooting

### Lỗi "tkinter not found"

```bash
# Ubuntu/Debian
sudo apt install python3-tk

# Fedora/RHEL
sudo dnf install python3-tkinter

# Arch Linux
sudo pacman -S tk
```

### Lỗi quyền thực thi

```bash
# Cấp quyền cho script
chmod +x run_gui.sh
chmod +x ../shell-scripting/**/*.sh
chmod +x ../smp-programming/demo.sh
```

### Process không dừng

- Click "Dừng Process" nhiều lần
- Process sẽ được terminate sau 2 giây
- Nếu không dừng, process sẽ bị kill (force)

## Lưu Ý

- Một số script cần quyền sudo, terminal sẽ yêu cầu password
- Compile code C cần có gcc và make đã cài đặt
- Kernel modules cần quyền root để load/unload
- Nên chạy trên Ubuntu/Linux để tương thích tốt nhất

## Phát Triển

File `app.py` được chia thành các phần chính:

- `__init__`: Khởi tạo GUI và biến
- `setup_ui`: Thiết lập giao diện chính
- `setup_*_tab`: Thiết lập từng tab
- `run_*`: Các hàm chạy lệnh
- `compile_*`: Các hàm compile
- `_run_command`: Hàm core chạy subprocess với realtime output
- `send_input`, `stop_process`: Tương tác với process

## License

Phần của dự án Kernel Linux - Bài Tập Lớn Lập Trình Hệ Thống
