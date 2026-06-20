# Ví dụ Sửa Lỗi Bằng Atomic (ex02)

Ví dụ này minh họa cách triển khai **đúng** của `release_resource()` sử dụng atomic operations để ngăn chặn race conditions.

## Giải Pháp

Sử dụng `atomic_fetch_sub` của C11, thao tác atomic này:
1. Lấy giá trị hiện tại
2. Giảm nó đi
3. Trả về giá trị CŨ

Điều này mô phỏng hành vi của `atomic_dec_and_test` trong kernel.

## Cấu Trúc Code

```c
void release_resource(void)
{
    int old_value = atomic_fetch_sub(&counter, 1);
    
    if (old_value == 1)  // Chúng ta vừa giảm từ 1 xuống 0
        free_resource();
}
```

## Biên Dịch và Chạy

```bash
make                # Biên dịch ví dụ
./atomic_fix        # Chạy một lần
make test           # Chạy 5 lần - nên LUÔN LUÔN thành công
```

## Kết Quả Mong Đợi

Tài nguyên sẽ **luôn** được giải phóng đúng một lần:

```
✓ SUCCESS: Resource freed exactly once (no race condition)
```

## Tài Liệu Tham Khảo

Xem **§ Atomic Operations** trong `smp-lecture.html` để hiểu lý thuyết nền tảng.

## So Sánh với ex01

Chạy cả hai ví dụ cùng lúc để thấy sự khác biệt:
- **ex01**: Race condition (tài nguyên bị giải phóng nhiều lần)
- **ex02**: Atomic operations (tài nguyên được giải phóng đúng một lần)
