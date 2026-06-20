# Ví dụ Spin Lock Cơ Bản (ex04)

Ví dụ này minh họa cơ chế spin lock cơ bản sử dụng atomic test-and-set operations, mô phỏng hành vi của lệnh `lock bts` trên x86.

## Khái Niệm

Spin lock là primitive đồng bộ hóa đơn giản nhất cho hệ thống SMP. Khi một luồng không thể giành được lock, nó **spin** (busy-wait) trong vòng lặp cho đến khi lock trở nên khả dụng.

## Triển Khai x86

Trong assembly x86:

```asm
spin_lock:
    lock bts [my_lock], 0   ; atomic bit-test-and-set
    jc spin_lock            ; spin nếu carry (lock đã bị chiếm)

; critical section

spin_unlock:
    mov [my_lock], 0
```

## Hành Vi Lệnh BTS

`bts dts, src` - bit test and set:
1. `CF <- dts[src]` - Copy bit vào carry flag
2. `dts[src] <- 1` - Set bit

Tiền tố `LOCK` đảm bảo tính atomic qua các CPU cores.

## Cấu Trúc Code

```c
void spin_lock(spinlock_t *lock)
{
    while (atomic_flag_test_and_set(&lock->lock)) {
        /* Spin - lock đang bị giữ bởi luồng khác */
    }
}

void spin_unlock(spinlock_t *lock)
{
    atomic_flag_clear(&lock->lock);
}
```

## Biên Dịch và Chạy

```bash
make                # Biên dịch ví dụ
./spinlock_basic    # Chạy demo
```

## Kết Quả Mong Đợi

Ví dụ tạo 4 luồng, mỗi luồng tăng counter chung 5 lần. Bạn sẽ thấy:
- Các luồng vào/ra critical sections
- Counter cuối cùng: 20 (đúng)
- Thống kê về lock contention (tổng số lần spin)

## Lock Contention

Thống kê cho thấy bao nhiêu lần các luồng phải spin chờ lock. Contention cao hơn = nhiều spins hơn = nhiều chu kỳ CPU bị lãng phí hơn.

## Tài Liệu Tham Khảo

Xem **§ Spin Locks** trong `smp-lecture.html` để hiểu lý thuyết nền tảng.

## Vấn Đề Hiệu Năng

Triển khai cơ bản này có vấn đề **cache thrashing** (được sửa trong ex05). Mỗi lần spin gây ra một phép ghi vào biến lock, làm vô hiệu hóa cache line trên tất cả các CPU.
