# Ví dụ Race Condition (ex01)

Ví dụ này minh họa race condition cổ điển trong quản lý tài nguyên, cụ thể là lỗi trong hàm `release_resource()` như đã thảo luận trong bài giảng SMP.

## Vấn Đề

Khi nhiều luồng đồng thời giảm một biến đếm chung và kiểm tra xem nó có bằng 0 không, race condition có thể xảy ra:

1. Luồng A giảm counter xuống 1
2. Luồng A bị preempt trước khi kiểm tra
3. Luồng B giảm counter xuống 0 và giải phóng tài nguyên
4. Luồng A tiếp tục, thấy counter bằng 0, và giải phóng tài nguyên **lần nữa**

## Cấu Trúc Code

```c
void release_resource(void)
{
    counter--;              // Phép toán không atomic
    
    if (!counter)           // Race condition ở đây!
        free_resource();
}
```

## Biên Dịch và Chạy

```bash
make                # Biên dịch ví dụ
./race_condition    # Chạy một lần
make test           # Chạy 5 lần để tăng khả năng kích hoạt lỗi
```

## Kết Quả Mong Đợi

Khi race condition được kích hoạt, bạn sẽ thấy:

```
!! BUG DETECTED: Resource freed multiple times!
```

## Tài Liệu Tham Khảo

Xem **§ Synchronization Basics** trong `smp-lecture.html` để hiểu lý thuyết nền tảng.

## Sửa Lỗi

Lỗi này được sửa trong ví dụ **ex02** sử dụng atomic operations.
