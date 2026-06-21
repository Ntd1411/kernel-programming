# Ví dụ Tắt Ngắt IRQ (ex03)

Ví dụ này minh họa cơ chế tắt/bật ngắt để đồng bộ hóa, mô phỏng các API của kernel là `local_irq_disable`, `local_irq_enable`, `local_irq_save`, và `local_irq_restore`.

## Khái Niệm

Trong kernel, việc tắt ngắt ngăn chặn truy cập đồng thời trên hệ thống đơn nhân:
- `local_irq_disable()` - Tắt ngắt
- `local_irq_enable()` - Bật ngắt
- `local_irq_save(flags)` - Lưu trạng thái hiện tại và tắt
- `local_irq_restore(flags)` - Khôi phục trạng thái trước đó

## Mô Phỏng User-Space

Vì không thể tắt ngắt phần cứng trong user-space, ví dụ này mô phỏng khái niệm bằng cách sử dụng **signal masking**:
- Signals hoạt động như "ngắt"
- `pthread_sigmask()` hoạt động như enable/disable ngắt

## Cấu Trúc Code

```c
void local_irq_disable(void)
{
    sigset_t set;
    sigfillset(&set);
    pthread_sigmask(SIG_BLOCK, &set, NULL);  // Chặn tất cả signals
}

void local_irq_save(sigset_t *flags)
{
    sigset_t set;
    sigfillset(&set);
    pthread_sigmask(SIG_BLOCK, &set, flags);  // Lưu & chặn
}
```

## Biên Dịch và Chạy

```bash
make                # Biên dịch ví dụ
./irq_disable       # Chạy demo
```

## Kết Quả Mong Đợi

Bạn sẽ thấy ba minh họa:
1. Disable/enable cơ bản
2. Save/restore (phương pháp được ưu tiên)
3. Critical sections lồng nhau

## Tài Liệu Tham Khảo

Xem **§ Disabling Preemption (Interrupts)** trong `smp-lecture.html` để hiểu lý thuyết nền tảng.

## Tại Sao Save/Restore Được Ưu Tiên

Sử dụng `local_irq_save`/`local_irq_restore` an toàn hơn so với enable/disable đơn thuần vì nó bảo toàn trạng thái ngắt, ngăn chặn việc vô tình bật lại trong các critical sections lồng nhau.
