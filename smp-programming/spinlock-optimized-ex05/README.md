# Ví dụ Spin Lock Tối Ưu (ex05)

Ví dụ này minh họa triển khai **spin lock tối ưu** giảm cache thrashing bằng cách sử dụng tối ưu hóa read-first và lệnh PAUSE.

## Vấn Đề với Naive Spin Locks

Spin lock naive từ ex04 gây ra **cache thrashing**:
- Mỗi luồng spin bằng cách liên tục thử atomic write operations
- Theo giao thức MESI, các phép ghi làm vô hiệu hóa cache line trong tất cả các CPU khác
- Điều này lãng phí băng thông bus bộ nhớ và làm chậm lock holder

## Tối Ưu Hóa

Phiên bản tối ưu sử dụng phương pháp hai bước:

```asm
spin_lock:
    rep ; nop           ; Lệnh PAUSE
    test lock_addr, 1   ; Kiểm tra READ-ONLY không atomic
    jnz spin_lock       ; Tiếp tục spin nếu bị khóa
    lock bts lock_addr  ; Chỉ thử giành khi có vẻ free
    jc spin_lock
```

**Ý tưởng chính:** Các phép đọc không atomic giữ cache line ở trạng thái **SHARED**. Chỉ thử atomic write khi lock có vẻ free.

## Lệnh PAUSE

Trên x86, lệnh `PAUSE`:
- Gợi ý cho CPU rằng chúng ta đang trong vòng lặp spin-wait
- Giảm tiêu thụ điện năng
- Tránh pipeline flushes từ memory order violations
- Cải thiện hiệu năng trên các CPU hyper-threaded

## Cấu Trúc Code

```c
void spin_lock_optimized(spinlock_t *lock)
{
    while (1) {
        /* Kiểm tra read-only - giữ cache line shared */
        while (atomic_load(&lock->lock) != 0) {
            cpu_pause();  /* Lệnh PAUSE */
        }
        
        /* Lock có vẻ free, thử giành atomically */
        if (atomic_compare_exchange_weak(&lock->lock, &expected, 1)) {
            break;  /* Thành công! */
        }
    }
}
```

## Biên Dịch và Chạy

```bash
make                      # Biên dịch ví dụ
./spinlock_optimized      # Chạy demo
```

## Kết Quả Mong Đợi

Ví dụ chạy cả hai phiên bản naive và optimized rồi so sánh:
- Tổng số spins (nên tương tự nhau)
- Thời gian thực thi (optimized nên nhanh hơn)
- Hành vi cache (optimized giảm invalidations)

## Lợi Ích Hiệu Năng

Trong hệ thống SMP thực tế, phiên bản tối ưu cho thấy:
- **Giảm bus traffic** - ít cache line invalidations hơn
- **Khả năng mở rộng tốt hơn** - hiệu năng cải thiện với nhiều CPU hơn
- **Tiêu thụ điện năng thấp hơn** - lệnh PAUSE giảm năng lượng lãng phí

## Tài Liệu Tham Khảo

Xem **§ Optimized Spin Locks** và **§ Cache Coherency** trong `smp-lecture.html` để hiểu lý thuyết nền tảng.

## So Sánh với ex04

- **ex04**: Triển khai naive, mỗi spin gây ra write
- **ex05**: Triển khai tối ưu, spins sử dụng reads cho đến khi lock free
