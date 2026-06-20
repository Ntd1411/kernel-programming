# Ví dụ Mutex Lock Slow Path (ex08)

Ví dụ này minh họa **slow path** của `mutex_lock()` nơi các luồng phải sleep khi mutex bị tranh chấp. Nó mô phỏng quản lý wait queue của kernel.

## Khái Niệm

Khi fast path thất bại (mutex đã bị giữ), luồng phải:
1. Thêm chính nó vào **wait queue**
2. Sleep (từ bỏ CPU)
3. Thức dậy khi mutex có thể khả dụng
4. Thử giành lại
5. Xóa chính nó khỏi wait queue khi thành công

## Thuật Toán Slow Path

```c
void __mutex_lock_slowpath(struct mutex *lock)
{
    spin_lock(&lock->wait_lock);          /* Bảo vệ wait_list */
    list_add_tail(&waiter.list, &lock->wait_list);  /* Tham gia hàng đợi */
    waiter.task = current;
    
    for (;;) {
        if (__mutex_trylock(lock))        /* Thử giành */
            goto acquired;
        
        spin_unlock(&lock->wait_lock);
        set_current_state(TASK_SLEEPING); /* Đi ngủ */
        schedule();                       /* Từ bỏ CPU */
        spin_lock(&lock->wait_lock);
    }
    
acquired:
    __set_current_state(TASK_RUNNING);
    mutex_remove_waiter(lock, &waiter, current);
    spin_unlock(&lock->wait_lock);
}
```

## Quản Lý Wait Queue

`wait_lock` (một spin lock) bảo vệ wait queue:
- Critical section ngắn (chỉ thao tác danh sách)
- An toàn khi dùng spin lock ở đây (không sleep khi giữ nó)
- Ngăn race khi nhiều luồng truy cập wait queue

## Biên Dịch và Chạy

```bash
make                    # Biên dịch ví dụ
./mutex_lock_slow       # Chạy demo
```

## Kết Quả Mong Đợi

Bạn sẽ thấy:
- Các luồng trải qua contention cao
- Nhiều lần giành slow path
- Thời gian chờ cho thấy các luồng đang sleeping
- **Tỷ lệ slow path**: Phần trăm các lần giành cần phải sleep

## Những Điểm Quan Trọng

- **Slow path tránh lãng phí CPU**: Các luồng sleep thay vì spin
- **Wait queue là FIFO**: Thứ tự công bằng của các waiter
- **Optimistic spinning**: Kernel thực sự spin ngắn trước khi sleep (không hiển thị ở đây)

## Tài Liệu Tham Khảo

Xem **§ Mutexes** trong `smp-lecture.html`, cụ thể phần slow path (VD 08 trong audit).

## So Sánh

- **ex07 (fast path)**: Giành atomic, không sleep
- **ex08 (slow path)**: Sleep và chờ, hiệu quả CPU khi có contention
