# Inter-Process Communication (IPC) Basics

Hướng dẫn sử dụng các phương thức IPC cơ bản trong Linux.

## Tổng quan

File `ipc_basics.c` demo 4 phương thức IPC chính:

1. **Pipes** - Kênh truyền dữ liệu một chiều
2. **Message Queues** - Hàng đợi tin nhắn có cấu trúc
3. **Shared Memory** - Vùng nhớ dùng chung
4. **Semaphores** - Đồng bộ hóa truy cập

## Biên dịch

```bash
make ipc_basics
# hoặc
gcc -o ipc_basics ipc_basics.c -lrt -pthread
```

## Chạy chương trình

```bash
./ipc_basics
```

## Các ví dụ trong chương trình

### 1. Unnamed Pipe (pipe)
- Giao tiếp một chiều giữa cha và con
- Tự động đóng khi tiến trình kết thúc
- Đơn giản nhất nhưng chỉ dùng cho tiến trình liên quan

```c
int pipefd[2];
pipe(pipefd);
// pipefd[0] - đọc
// pipefd[1] - ghi
```

### 2. Two-way Communication
- Sử dụng 2 pipes để giao tiếp hai chiều
- pipe1: cha -> con
- pipe2: con -> cha

### 3. Named Pipe (FIFO)
- Có tên file trong hệ thống
- Tiến trình không liên quan có thể giao tiếp
- Tồn tại cho đến khi bị xóa

```bash
mkfifo /tmp/my_fifo
```

### 4. Message Queue - Cơ bản
- Gửi/nhận tin nhắn có cấu trúc
- FIFO order theo mặc định
- Persistent cho đến khi xóa

```c
struct msg_buffer {
    long msg_type;
    char msg_text[100];
};
```

### 5. Message Queue - Priority
- Sử dụng msg_type để ưu tiên
- Nhận tin nhắn theo type cụ thể
- Type cao hơn = priority cao hơn

### 6. System V Shared Memory
- Vùng nhớ dùng chung giữa các tiến trình
- Truy cập nhanh nhất
- Cần semaphore để đồng bộ

```c
shmid = shmget(key, size, IPC_CREAT | 0666);
ptr = shmat(shmid, NULL, 0);
```

### 7. POSIX Shared Memory
- API hiện đại hơn System V
- Sử dụng mmap() để ánh xạ
- Dễ sử dụng hơn

```c
shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
```

### 8. System V Semaphore
- Đồng bộ hóa truy cập tài nguyên
- P operation: wait/lock (sem_op = -1)
- V operation: signal/unlock (sem_op = +1)

```c
struct sembuf sb;
sb.sem_op = -1;  // Wait
semop(semid, &sb, 1);
// Critical section
sb.sem_op = 1;   // Signal
semop(semid, &sb, 1);
```

### 9. POSIX Named Semaphore
- API đơn giản hơn System V
- Tương thích tốt với threading

```c
sem_t *sem = sem_open(name, O_CREAT, 0666, 1);
sem_wait(sem);    // Lock
// Critical section
sem_post(sem);    // Unlock
```

## So sánh các phương thức IPC

| Phương thức | Tốc độ | Phức tạp | Use case |
|-------------|--------|----------|----------|
| Pipe | Trung bình | Đơn giản | Tiến trình liên quan, stream data |
| FIFO | Trung bình | Đơn giản | Tiến trình không liên quan |
| Message Queue | Trung bình | Trung bình | Tin nhắn có cấu trúc, priority |
| Shared Memory | Nhanh nhất | Phức tạp | Dữ liệu lớn, cần tốc độ |
| Semaphore | N/A | Trung bình | Đồng bộ hóa |

## Lưu ý quan trọng

### Cleanup Resources
- Luôn xóa IPC objects sau khi dùng xong:
  - Message queue: `msgctl(msgid, IPC_RMID, NULL)`
  - Shared memory: `shmctl(shmid, IPC_RMID, NULL)`
  - Semaphore: `semctl(semid, 0, IPC_RMID)`
  - FIFO: `unlink(fifo_name)`
  - POSIX shm: `shm_unlink(name)`
  - POSIX sem: `sem_unlink(name)`

### Kiểm tra IPC objects
```bash
# Xem message queues
ipcs -q

# Xem shared memory
ipcs -m

# Xem semaphores
ipcs -s

# Xóa tất cả IPC của user
ipcrm -a
```

### Race Conditions
- Shared memory cần semaphore để tránh race condition
- Message queue thread-safe theo mặc định
- Pipe thread-safe cho read/write riêng biệt

### Permissions
- IPC objects có permissions như file (0666)
- Cần quyền phù hợp để truy cập

## Troubleshooting

### "No such file or directory" với FIFO
- Đảm bảo FIFO được tạo trước khi mở
- Kiểm tra quyền truy cập

### "Resource busy" khi xóa IPC
- Đảm bảo tất cả tiến trình đã detach
- Dùng `ipcs` và `ipcrm` để cleanup

### Deadlock với semaphore
- Luôn unlock semaphore trong mọi trường hợp
- Sử dụng timeout nếu cần

## Tài liệu tham khảo

- `man 2 pipe`
- `man 3 mkfifo`
- `man 2 msgget`
- `man 2 shmget`
- `man 2 semget`
- `man 7 shm_overview`
- `man 7 sem_overview`
