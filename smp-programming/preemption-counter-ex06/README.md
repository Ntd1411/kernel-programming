# Ví dụ Preemption Counter (ex06)

Ví dụ này minh họa cơ chế preemption counter của kernel, theo dõi và kiểm soát trạng thái preemption qua các ngữ cảnh khác nhau (process, softirq, hardirq, NMI).

## Khái Niệm

Kernel sử dụng một biến `preempt_count` duy nhất trên mỗi CPU để theo dõi nhiều loại trạng thái preemption-disabled sử dụng các dải bit khác nhau.

## Bố Trí Bit

```
Bits 0-7:   Preemption count (0-255)
Bits 8-15:  Softirq count (0-255)
Bits 16-19: Hardirq count (0-15)
Bit  20:    NMI count (0-1)
```

## Các Macro Quan Trọng

```c
#define PREEMPT_OFFSET  (1UL << 0)   /* 0x000001 */
#define SOFTIRQ_OFFSET  (1UL << 8)   /* 0x000100 */
#define HARDIRQ_OFFSET  (1UL << 16)  /* 0x010000 */
#define NMI_OFFSET      (1UL << 20)  /* 0x100000 */

#define preempt_disable()   add_preempt_count(PREEMPT_OFFSET)
#define local_bh_disable()  add_preempt_count(SOFTIRQ_OFFSET)

#define irq_count()         (preempt_count() & (HARDIRQ_MASK | SOFTIRQ_MASK))
#define in_interrupt()      (irq_count() != 0)
```

## Hành Vi do_softirq()

```c
asmlinkage void do_softirq(void)
{
    if (in_interrupt()) return;  /* Đã ở trong interrupt context */
    /* ... xử lý softirqs ... */
}
```

Kiểm tra quan trọng ngăn chặn xử lý softirq đệ quy khi đã ở trong hardirq hoặc softirq context.

## Biên Dịch và Chạy

```bash
make                        # Biên dịch ví dụ
./preemption_counter        # Chạy demo
```

## Kết Quả Mong Đợi

Bạn sẽ thấy các minh họa về:
1. **Preemption disable** - Tăng/giảm preempt count
2. **Bottom-half disable** - Set softirq bits, kích hoạt `in_interrupt()`
3. **Simulated IRQ handler** - Hardirq context ngăn softirq processing
4. **Combined disable** - Nhiều loại disable tương tác

## Những Điểm Quan Trọng

- **Nested disable/enable hoạt động đúng** - Counter tăng/giảm đúng cách
- **`in_interrupt()` kiểm tra cả hardirq và softirq bits** - Trả về true nếu một trong hai được set
- **`do_softirq()` sẽ không chạy trong interrupt context** - Ngăn chặn vấn đề reentrancy

## Tài Liệu Tham Khảo

Xem **§ Process and Interrupt Context Synchronization** trong `smp-lecture.html`, cụ thể phần preemption counter (VD 06 trong audit).
