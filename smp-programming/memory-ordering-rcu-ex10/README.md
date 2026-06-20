# Ví dụ Memory Ordering và RCU (ex10)

Ví dụ này minh họa các khái niệm **memory ordering** và đồng bộ hóa **RCU (Read-Copy-Update)**, cho thấy kernel đảm bảo thứ tự đúng của các phép toán bộ nhớ như thế nào.

## Ba Khái Niệm

### 1. Compiler Barriers
Ngăn **compiler** sắp xếp lại các lệnh:
```c
x = 1;
barrier();  /* asm volatile("": : :"memory") */
y = x;      /* Compiler không thể di chuyển dòng này lên trên barrier */
```

### 2. Memory Barriers
Ngăn **CPU** sắp xếp lại các phép toán bộ nhớ:
- `mb()`: Full memory barrier (mfence trên x86)
- `rmb()`: Read memory barrier (lfence)
- `wmb()`: Write memory barrier (sfence)
- `smp_*`: Phiên bản SMP (giống trên x86, nhẹ hơn trên một số kiến trúc)

### 3. RCU (Read-Copy-Update)
Đồng bộ hóa lock-free cho phép đọc đồng thời trong khi cập nhật:
- **Readers**: Dùng `rcu_read_lock()` / `rcu_read_unlock()` (rất nhanh, không có atomic ops)
- **Writers**: Dùng `rcu_assign_pointer()` để publish thay đổi
- **Synchronization**: `synchronize_rcu()` chờ tất cả readers kết thúc

## Mô Hình Bộ Nhớ x86-64

x86 có **strong memory model**:
- Loads không được sắp xếp lại với loads
- Stores không được sắp xếp lại với stores
- Stores không được sắp xếp lại với loads trước đó
- **Chỉ** loads có thể được sắp xếp lại với stores trước đó tới các địa chỉ khác

Đây là lý do tại sao nhiều barriers của kernel là NOPs trên x86 nhưng quan trọng trên ARM/PowerPC.

## Đảm Bảo RCU

```c
/* Writer */
new->data = 42;             /* Khởi tạo */
rcu_assign_pointer(ptr, new);  /* Publish (với wmb) */

/* Reader */
p = rcu_dereference(ptr);   /* Fetch (với dependency barrier) */
if (p) value = p->data;     /* Đảm bảo thấy 42 */
```

Các barriers đảm bảo readers thấy dữ liệu đã khởi tạo đầy đủ.

## Biên Dịch và Chạy

```bash
make                         # Biên dịch ví dụ
./memory_ordering_rcu        # Chạy demo
```

## Kết Quả Mong Đợi

Bạn sẽ thấy:
- Giải thích về compiler và memory barriers
- Các phép toán RCU list với readers và writers đồng thời
- Writers thêm/xóa nodes trong khi readers duyệt
- Tất cả phép toán hoàn thành không có data races

## Những Điểm Quan Trọng

- **Compiler barriers** ngăn sắp xếp lại lệnh tại compile time
- **Memory barriers** đảm bảo thứ tự visible với các CPU khác
- **RCU** cho phép reads lock-free: readers không bao giờ chặn writers, writers không bao giờ chặn readers
- **x86 strong ordering** có nghĩa cần ít barriers hơn so với kiến trúc weak-ordered
- **synchronize_rcu()** là chìa khóa: đảm bảo tất cả readers thấy dữ liệu cũ trước khi giải phóng

## Tài Liệu Tham Khảo

Xem **§ Memory Ordering** và **§ RCU** trong `smp-lecture.html` (VD 10 trong audit).

## Các Khái Niệm Liên Quan

- **acquire/release semantics**: Dùng trong atomic operations (ex02, ex07)
- **memory_order_consume**: Dependency ordering (dùng trong rcu_dereference)
- **memory_order_release**: Publish ordering (dùng trong rcu_assign_pointer)
