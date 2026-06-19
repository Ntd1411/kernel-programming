# Network Programming

Các ví dụ về lập trình mạng (network programming) trong Linux, bao gồm I/O multiplexing, raw sockets, và quản lý network interface.

## Tổng quan

Network programming trong Linux cung cấp các cơ chế để:
- Xử lý nhiều kết nối đồng thời (I/O multiplexing)
- Truy cập packet ở mức thấp (raw sockets)
- Bắt và phân tích gói tin (packet sniffing)
- Quản lý network interfaces (ioctl)

## Danh sách chương trình

### 1. I/O Multiplexing

#### select_server.c
Server TCP sử dụng `select()` để xử lý nhiều clients.

**Đặc điểm:**
- Hỗ trợ tối đa 1024 file descriptors (FD_SETSIZE)
- Phải quét toàn bộ fd_set mỗi lần
- Portable trên hầu hết các hệ thống UNIX

**Build và chạy:**
```bash
make select_server
./select_server 8080
# Terminal khác: telnet localhost 8080
```

#### poll_server.c
Server TCP sử dụng `poll()` để xử lý nhiều clients.

**Đặc điểm:**
- Không giới hạn số lượng file descriptors
- Hiệu năng tốt hơn select với số lượng connections lớn
- API đơn giản hơn select

**Build và chạy:**
```bash
make poll_server
./poll_server 8081
# Terminal khác: telnet localhost 8081
```

#### epoll_server.c
Server TCP sử dụng `epoll()` để xử lý nhiều clients (Linux specific).

**Đặc điểm:**
- Hiệu năng cao nhất, phù hợp cho hệ thống lớn
- Chỉ thông báo FDs có sự kiện (edge-triggered hoặc level-triggered)
- Chỉ có trên Linux

**Build và chạy:**
```bash
make epoll_server
./epoll_server 8082
# Terminal khác: telnet localhost 8082
```

### 2. Raw Sockets

#### raw_socket.c
Tạo và gửi raw IP packets.

**Đặc điểm:**
- Xây dựng IP header thủ công
- Bỏ qua TCP/UDP layer
- Cần quyền root (CAP_NET_RAW)

**Build và chạy:**
```bash
make raw_socket
sudo ./raw_socket 8.8.8.8
```

#### packet_sniffer.c
Bắt và phân tích các gói tin ở mức data link layer.

**Đặc điểm:**
- Sử dụng AF_PACKET socket
- Bắt tất cả gói tin trên interface
- Phân tích Ethernet, IP, TCP, UDP headers
- Cần quyền root

**Build và chạy:**
```bash
make packet_sniffer
sudo ./packet_sniffer eth0
# Hoặc bắt tất cả interfaces:
sudo ./packet_sniffer
```

### 3. Network Interface Management

#### network_interface.c
Quản lý network interfaces thông qua ioctl() system calls.

**Chức năng:**
- Liệt kê tất cả interfaces
- Đọc thông tin: IP, MAC, netmask, broadcast, MTU
- Set IP address và netmask
- Bật/tắt interface (UP/DOWN)
- Demo sử dụng system() với ip commands

**Build và chạy:**
```bash
make network_interface
# Chế độ read-only:
./network_interface
# Chế độ đầy đủ (cần root):
sudo ./network_interface
```

**Xem chi tiết:** [NETWORK_INTERFACE.md](NETWORK_INTERFACE.md)

## So sánh I/O Multiplexing

| Tính năng | select() | poll() | epoll() |
|-----------|----------|--------|---------|
| Giới hạn FDs | 1024 | Không giới hạn | Không giới hạn |
| Hiệu năng | O(n) | O(n) | O(1) |
| Portability | Cao | Cao | Chỉ Linux |
| Sử dụng | Ít connections | Trung bình | Nhiều connections |
| Edge-triggered | Không | Không | Có |

## System Calls chính

### I/O Multiplexing

```c
// select
int select(int nfds, fd_set *readfds, fd_set *writefds, 
           fd_set *exceptfds, struct timeval *timeout);

// poll
int poll(struct pollfd *fds, nfds_t nfds, int timeout);

// epoll
int epoll_create(int size);
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
int epoll_wait(int epfd, struct epoll_event *events, 
               int maxevents, int timeout);
```

### Raw Sockets

```c
// Tạo raw socket
int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);

// Packet socket
int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
```

### Network Interface

```c
// ioctl cho network interface
int ioctl(int sockfd, unsigned long request, struct ifreq *ifr);

// Lấy danh sách interfaces
int getifaddrs(struct ifaddrs **ifap);
```

## Build tất cả

```bash
cd system-programming/network-programming
make
```

Build từng chương trình riêng:
```bash
make select_server
make poll_server
make epoll_server
make raw_socket
make packet_sniffer
make network_interface
```

Clean up:
```bash
make clean
```

## Lưu ý quan trọng

### Quyền root

Các chương trình sau cần quyền root:
- `raw_socket` - Tạo raw IP packets
- `packet_sniffer` - Bắt packets ở data link layer
- `network_interface` - Set IP, up/down interfaces (chỉ các chức năng ghi)

### Sử dụng capabilities thay vì root

Thay vì chạy với sudo, có thể cấp capabilities cụ thể:

```bash
# Cho phép raw socket
sudo setcap cap_net_raw+ep ./raw_socket
sudo setcap cap_net_raw+ep ./packet_sniffer

# Cho phép thay đổi network config
sudo setcap cap_net_admin+ep ./network_interface
```

### Firewall

Một số chương trình có thể bị chặn bởi firewall:
```bash
# Kiểm tra firewall
sudo iptables -L

# Tạm thời tắt firewall (cẩn thận!)
sudo systemctl stop firewalld  # CentOS/RHEL
sudo ufw disable               # Ubuntu
```

## Testing

### Test I/O Multiplexing Servers

```bash
# Terminal 1: Chạy server
./select_server 8080

# Terminal 2: Kết nối với telnet
telnet localhost 8080

# Terminal 3: Kết nối client thứ 2
telnet localhost 8080

# Gõ tin nhắn ở mỗi terminal để test
```

### Test với nhiều clients

```bash
# Tạo test script
cat > test_clients.sh << 'EOF'
#!/bin/bash
for i in {1..10}; do
    (echo "Client $i"; sleep 1) | nc localhost 8080 &
done
wait
EOF

chmod +x test_clients.sh
./test_clients.sh
```

### Test Raw Socket

```bash
# Chạy packet sniffer trước
sudo ./packet_sniffer eth0

# Terminal khác: gửi raw packet
sudo ./raw_socket 192.168.1.1

# Xem packet trong output của sniffer
```

### Test Network Interface

```bash
# Xem thông tin tất cả interfaces
./network_interface
# Chọn option 1

# Xem chi tiết một interface
./network_interface
# Chọn option 2, nhập tên interface (vd: eth0)

# Test với quyền root
sudo ./network_interface
# Chọn option 5 để set IP (cẩn thận!)
```

## Debugging

### Xem network traffic

```bash
# Tcpdump
sudo tcpdump -i eth0 -n

# Wireshark (GUI)
sudo wireshark
```

### Kiểm tra ports đang listen

```bash
# Netstat
netstat -tlnp

# ss (modern alternative)
ss -tlnp

# lsof
sudo lsof -i :8080
```

### Xem active connections

```bash
# Tất cả connections
netstat -an

# Chỉ TCP
netstat -ant

# Với process info
sudo netstat -tanp
```

### Trace system calls

```bash
# Trace chương trình
strace ./select_server 8080

# Chỉ trace network syscalls
strace -e trace=network ./select_server 8080

# Trace process đang chạy
sudo strace -p <PID>
```

## Troubleshooting

### Lỗi: Address already in use

```
Nguyên nhân: Port đang được sử dụng bởi process khác
Giải pháp:
1. Đổi port khác
2. Hoặc kill process đang dùng port:
   sudo lsof -ti:8080 | xargs kill -9
3. Hoặc set SO_REUSEADDR trong code
```

### Lỗi: Permission denied (raw socket)

```
Nguyên nhân: Thiếu quyền root hoặc CAP_NET_RAW
Giải pháp:
1. sudo ./raw_socket
2. Hoặc: sudo setcap cap_net_raw+ep ./raw_socket
```

### Lỗi: Cannot find device

```
Nguyên nhân: Interface name không đúng
Giải pháp: Kiểm tra với ip link show hoặc ifconfig
```

### Server không nhận connections

```
Kiểm tra:
1. Server có đang chạy? ps aux | grep server
2. Port có đang listen? netstat -tlnp | grep 8080
3. Firewall có chặn? sudo iptables -L
4. Bind address đúng? (0.0.0.0 cho tất cả interfaces)
```

## Performance Tips

### epoll cho high-performance

```c
// Sử dụng edge-triggered mode
event.events = EPOLLIN | EPOLLET;

// Non-blocking sockets
int flags = fcntl(sockfd, F_GETFL, 0);
fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
```

### Buffer size tuning

```c
// Tăng buffer size
int bufsize = 1024 * 1024;  // 1MB
setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
```

### TCP_NODELAY

```c
// Tắt Nagle algorithm để giảm latency
int flag = 1;
setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
```

## Tham khảo

### Man pages
- `man 2 select` - select system call
- `man 2 poll` - poll system call
- `man 7 epoll` - epoll API
- `man 7 raw` - raw sockets
- `man 7 packet` - packet sockets
- `man 7 netdevice` - network device interface
- `man 2 ioctl` - ioctl system call

### Books
- "UNIX Network Programming" - W. Richard Stevens
- "Linux System Programming" - Robert Love
- "The Linux Programming Interface" - Michael Kerrisk

### Online resources
- Linux kernel documentation: /usr/src/linux/Documentation/networking/
- Beej's Guide to Network Programming

## Liên quan

- [Socket Programming](../socket-programming/) - Basic TCP/UDP sockets
- [Process Management](../process-management/) - Process và IPC
- [File Management](../file-management/) - File I/O operations

