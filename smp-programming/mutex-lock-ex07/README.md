# Ví dụ Mutex Lock Fast Path (ex07)

Ví dụ này minh họa **fast path** của `mutex_lock()` sử dụng atomic compare-and-exchange. Đây là trường hợp lạc quan khi mutex không bị tranh chấp.

## Khái Niệm

Mutexes sử dụng phương pháp hai đường dẫn để tối ưu hiệu suất:
- **Fast path**: Giành atomic khi mutex free (không có context switch)
- **Slow path**: Sleep và chờ khi mutex bị tranh chấp (xem ex08)

## Triển Khai Fast Path

```c
void mutex_lock(struct mutex *lock)
{
    might_sleep();  /* Kiểm tra debug - ta đang ở sleepable context */
    
    if (!__mutex_trylock_fast(lock))
        __mutex_lock_slowpath(lock);  /* Quay về slow path */
}

static inline bool __mutex_trylock_fast(struct mutex *lock)
{
    unsigned long curr = (unsigned long)current;
    
    if (!atomic_long_cmpxchg_acquire(&lock->owner, 0UL, curr))
        return true;  /* Thành công! */
    
    return false;  /* Mutex đã bị giữ - cần slow path */
}
```

## atomic_cmpxchg_acquire

Phép toán quan trọng là **atomic compare-and-exchange** với **acquire semantics**:
1. Atomically kiểm tra nếu `lock->owner == 0` (free)
2. Nếu có, set `lock->owner = current_thread`
3. Trả về success/failure

Memory order `acquire` đảm bảo tất cả reads/writes sau xảy ra **sau khi** giành được lock.

## Biên Dịch và Chạy

```bash
make            # Biên dịch ví dụ
./mutex_lock    # Chạy demo
```

## Kết Quả Mong Đợi

Bạn sẽ thấy:
- Các luồng giành và giải phóng mutex
- Counter cuối cùng: 12 (4 luồng × 3 lần lặp)
- **Thống kê fast path**: Tỷ lệ thành công cho thấy fast path hoạt động thường xuyên như thế nào

## Tỷ Lệ Thành Công Fast Path

- **Tỷ lệ cao**: Contention thấp, mutex thường free
- **Tỷ lệ thấp**: Contention cao, các luồng thường bị chặn

## Tài Liệu Tham Khảo

Xem **§ Mutexes** trong `smp-lecture.html`, cụ thể phần fast path (VD 07 trong audit).

## So Sánh với Spin Locks

- **Spin lock**: Đốt CPU khi chờ
- **Mutex (slow path)**: Sleep khi chờ (không lãng phí CPU)
- **Mutex (fast path)**: Không chờ đợi gì cả (giành atomic)
