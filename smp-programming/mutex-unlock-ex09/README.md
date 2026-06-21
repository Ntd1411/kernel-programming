# Ví dụ Mutex Unlock (ex09)

Ví dụ này minh họa cả **fast path** và **slow path** của `mutex_unlock()`, cho thấy kernel tối ưu trường hợp phổ biến trong khi vẫn đánh thức waiters đúng cách khi cần.

## Hai Đường Dẫn

### Fast Path (Không Có Waiters)
Khi không có luồng nào chờ, unlock chỉ là một phép xóa atomic:
```c
if (owner == current && !(owner & WAITERS_FLAG)) {
    owner = 0;  /* Atomic compare-exchange */
    return;     /* Xong! */
}
```

### Slow Path (Có Waiters)
Khi có waiters, ta phải đánh thức một:
```c
spin_lock(&lock->wait_lock);
waiter = list_first_entry(&lock->wait_list, ...);
wake_q_add(&wake_q, waiter->task);
owner = 0;
spin_unlock(&lock->wait_lock);
wake_up_q(&wake_q);  /* Thực sự đánh thức luồng */
```

## WAITERS Flag

Các bit thấp của trường `owner` lưu trữ flags:
- Bit 0: `MUTEX_FLAG_WAITERS` (0x01)

Điều này hoạt động vì `task_struct` được **cache-aligned** (64+ bytes), nên 7 bit thấp của con trỏ luôn bằng không và có thể dùng cho flags.

## Tối Ưu Wake Queue

Kernel thực tế sử dụng `wake_q` để trì hoãn wakeups:
1. Thêm waiters vào wake queue khi giữ wait_lock
2. Giải phóng wait_lock
3. Đánh thức tất cả luồng trong wake_q (không giữ lock)

Điều này giảm thiểu thời gian critical section.

## Biên Dịch và Chạy

```bash
make                # Biên dịch ví dụ
./mutex_unlock      # Chạy demo
```

## Kết Quả Mong Đợi

Bạn sẽ thấy:
- Nhiều luồng tranh chấp mutex
- Thống kê cho thấy số lần fast vs slow path unlock
- **Tỷ lệ fast path**: Phần trăm cho thấy fast path thành công thường xuyên như thế nào

Với contention cao, slow path chiếm ưu thế (waiters hiện diện hầu hết thời gian).

## Những Điểm Quan Trọng

- **Fast path rất quan trọng**: Hầu hết unlocks nên là fast (không có waiters)
- **Tối ưu waiters flag**: Kiểm tra waiters trước khi lấy wait_lock
- **Deferred wakeup**: wake_q giảm thiểu thời gian giữ wait_lock

## Tài Liệu Tham Khảo

Xem **§ Mutexes** trong `smp-lecture.html`, cụ thể triển khai unlock (VD 09 trong audit).

## So Sánh

- **ex07**: Fast path lock (giành lạc quan)
- **ex08**: Slow path lock (sleeping khi có contention)
- **ex09**: Unlock paths (xóa atomic nhanh vs đánh thức waiter)
