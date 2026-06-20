# Socket Programming Examples

## Tổng quan

Socket là endpoint cho giao tiếp mạng giữa các process, có thể trên cùng máy hoặc khác máy. Linux cung cấp socket API để lập trình mạng.

## Các loại Socket

### 1. Stream Sockets (SOCK_STREAM)
- Sử dụng TCP protocol
- Connection-oriented
- Đảm bảo thứ tự và độ tin cậy
- Hai chiều (full-duplex)

### 2. Datagram Sockets (SOCK_DGRAM)
- Sử dụng UDP protocol
- Connectionless
- Không đảm bảo thứ tự, có thể mất gói tin
- Nhanh hơn TCP

## Cấu trúc dự án

```
socket-programming/
├── tcp_server.c      - TCP server đơn giản (sequential)
├── tcp_client.c      - TCP client cơ bản
├── udp_server.c      - UDP server
├── udp_client.c      - UDP client
├── echo_server.c     - Echo server đa luồng
├── echo_client.c     - Echo client
├── Makefile          - Build script
└── README.md         - Tài liệu này
```

## Socket Flow Diagram

### TCP Client-Server Flow

```
SERVER                          CLIENT
------                          ------
socket()                        
  |
bind()
  |
listen()
  |                             socket()
  |                               |
accept() <-------------------  connect()
  |                               |
recv() <----------------------  send()
  |                               |
send() --------------------->  recv()
  |                               |
close()                        close()
```

### UDP Client-Server Flow

```
SERVER                          CLIENT
------                          ------
socket()                        socket()
  |                               |
bind()                            |
  |                               |
recvfrom() <------------------ sendto()
  |                               |
sendto() --------------------> recvfrom()
  |                               |
close()                        close()
```

## Các System Calls chính

### 1. socket() - Tạo socket endpoint

```c
int socket(int domain, int type, int protocol);
```

**Tham số:**
- `domain`: AF_INET (IPv4), AF_INET6 (IPv6), AF_UNIX (local)
- `type`: SOCK_STREAM (TCP), SOCK_DGRAM (UDP)
- `protocol`: thường là 0 (auto-select)

**Trả về:** File descriptor hoặc -1 nếu lỗi

### 2. bind() - Gắn địa chỉ vào socket

```c
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

**Mục đích:** Gán địa chỉ IP và port cho socket server

### 3. listen() - Lắng nghe kết nối (TCP only)

```c
int listen(int sockfd, int backlog);
```

**Tham số:**
- `backlog`: Số lượng kết nối chờ tối đa trong queue

### 4. accept() - Chấp nhận kết nối (TCP only)

```c
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

**Trả về:** File descriptor mới cho connection hoặc -1 nếu lỗi

### 5. connect() - Kết nối đến server (TCP)

```c
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

### 6. send()/recv() - Gửi/nhận dữ liệu (TCP)

```c
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
```

### 7. sendto()/recvfrom() - Gửi/nhận datagram (UDP)

```c
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen);

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen);
```

## Chi tiết các chương trình

### 1. tcp_server.c - TCP Server Sequential

**Đặc điểm:**
- Xử lý từng client tuần tự
- Đơn giản, dễ hiểu
- Không phù hợp cho nhiều client

**Biên dịch và chạy:**
```bash
gcc -o tcp_server tcp_server.c
./tcp_server 8080
```

**Các bước hoạt động:**
1. Tạo socket với SOCK_STREAM
2. Set SO_REUSEADDR để tránh "Address already in use"
3. Bind socket vào port
4. Listen với backlog queue
5. Loop: accept -> recv -> send -> close client

### 2. tcp_client.c - TCP Client

**Đặc điểm:**
- Kết nối đến TCP server
- Gửi/nhận dữ liệu tương tác
- Sử dụng stdin để nhập

**Biên dịch và chạy:**
```bash
gcc -o tcp_client tcp_client.c
./tcp_client localhost 8080
./tcp_client 127.0.0.1 8080
```

**Các bước hoạt động:**
1. Tạo socket
2. Chuyển IP string sang binary với inet_pton()
3. Connect đến server
4. Loop: nhập -> send -> recv -> hiển thị

### 3. udp_server.c - UDP Server

**Đặc điểm:**
- Connectionless, không cần accept
- Nhận datagram từ bất kỳ client nào
- Gửi response về địa chỉ nguồn

**Biên dịch và chạy:**
```bash
gcc -o udp_server udp_server.c
./udp_server 8081
```

**Các bước hoạt động:**
1. Tạo socket với SOCK_DGRAM
2. Bind socket
3. Loop: recvfrom (lấy cả địa chỉ client) -> xử lý -> sendto

### 4. udp_client.c - UDP Client

**Đặc điểm:**
- Không cần connect
- Gửi datagram trực tiếp
- Set timeout cho recv

**Biên dịch và chạy:**
```bash
gcc -o udp_client udp_client.c
./udp_client localhost 8081
```

**Timeout setting:**
```c
struct timeval tv;
tv.tv_sec = 2;   // 2 giây timeout
tv.tv_usec = 0;
setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
```

### 5. echo_server.c - Multi-threaded Echo Server

**Đặc điểm:**
- Xử lý nhiều client đồng thời
- Mỗi client có thread riêng
- Thread-safe với mutex
- Giới hạn số client tối đa

**Biên dịch và chạy:**
```bash
gcc -o echo_server echo_server.c -lpthread
./echo_server 8082
```

**Kiến trúc:**
```
Main Thread (accept loop)
    |
    +-- Client Thread 1 (handle_client)
    |
    +-- Client Thread 2 (handle_client)
    |
    +-- Client Thread 3 (handle_client)
    |
    ...
```

**Thread safety:**
- Sử dụng mutex để bảo vệ biến `client_count`
- Mỗi thread detach sau khi tạo
- Tự động cleanup khi client ngắt kết nối

### 6. echo_client.c - Echo Client

**Đặc điểm:**
- Client chuyên dụng cho echo server
- UI thân thiện với người dùng
- Xử lý EOF và quit commands
- Hiển thị welcome message

**Biên dịch và chạy:**
```bash
gcc -o echo_client echo_client.c
./echo_client localhost 8082
```

**Tính năng:**
- Nhận welcome message từ server
- Echo mọi tin nhắn gửi đi
- Gõ 'quit' hoặc 'exit' để thoát
- Nhấn Ctrl+D (EOF) để ngắt kết nối
- Xử lý lỗi mạng gracefully

## Build và Test

### Build tất cả

```bash
make
# hoặc
make all
```

### Build từng chương trình

```bash
make tcp_server
make tcp_client
make udp_server
make udp_client
make echo_server
make echo_client
```

### Clean

```bash
make clean
```

### Xem hướng dẫn

```bash
make help
make test
```

## Thực hành

### Bài 1: Test TCP Server/Client

**Terminal 1 (Server):**
```bash
./tcp_server 8080
```

**Terminal 2 (Client):**
```bash
./tcp_client localhost 8080
```

**Thử nghiệm:**
- Gửi tin nhắn từ client
- Xem server echo lại
- Quan sát server chỉ xử lý 1 client tại 1 thời điểm

### Bài 2: Test UDP Server/Client

**Terminal 1 (Server):**
```bash
./udp_server 8081
```

**Terminal 2 (Client):**
```bash
./udp_client localhost 8081
```

**Thử nghiệm:**
- Gửi datagram
- UDP không cần connection
- Test timeout khi tắt server

### Bài 3: Test Multi-threaded Echo Server

**Terminal 1 (Server):**
```bash
./echo_server 8082
```

**Terminal 2 (Client 1):**
```bash
./echo_client localhost 8082
```

**Terminal 3 (Client 2):**
```bash
./echo_client localhost 8082
```

**Terminal 4 (Client 3):**
```bash
./echo_client localhost 8082
```

**Thử nghiệm:**
- Mở nhiều client đồng thời
- Gửi tin nhắn từ các client khác nhau
- Quan sát server xử lý concurrent
- Kiểm tra client count
- Ngắt kết nối từng client

### Bài 4: Test với netcat

```bash
# Server
./tcp_server 8080

# Client bằng netcat
nc localhost 8080
```

### Bài 5: Monitor với tcpdump

```bash
# Terminal 1
sudo tcpdump -i lo port 8080 -A

# Terminal 2
./tcp_server 8080

# Terminal 3
./tcp_client localhost 8080
```

## Các vấn đề thường gặp

### 1. Address already in use

**Nguyên nhân:** Port chưa được release sau khi tắt server

**Giải pháp:**
```c
int opt = 1;
setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```

### 2. Connection refused

**Nguyên nhân:** Server chưa chạy hoặc sai port

**Giải pháp:**
- Kiểm tra server đang chạy: `netstat -an | grep 8080`
- Kiểm tra port đúng
- Kiểm tra firewall

### 3. Broken pipe

**Nguyên nhân:** Gửi dữ liệu khi kết nối đã đóng

**Giải pháp:**
- Kiểm tra return value của send/recv
- Xử lý SIGPIPE signal

### 4. Resource temporarily unavailable

**Nguyên nhân:** Non-blocking socket chưa sẵn sàng

**Giải pháp:**
- Sử dụng select/poll/epoll
- Hoặc chuyển về blocking mode

### 5. Too many open files

**Nguyên nhân:** Quên close socket hoặc đạt giới hạn

**Giải pháp:**
- Luôn close socket sau khi dùng
- Kiểm tra giới hạn: `ulimit -n`
- Tăng giới hạn: `ulimit -n 4096`

## Socket Options

### SO_REUSEADDR - Cho phép reuse address

```c
int opt = 1;
setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```

**Mục đích:** Tránh lỗi "Address already in use" khi restart server

### SO_RCVTIMEO - Set receive timeout

```c
struct timeval tv;
tv.tv_sec = 5;
tv.tv_usec = 0;
setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
```

### SO_SNDTIMEO - Set send timeout

```c
struct timeval tv;
tv.tv_sec = 5;
tv.tv_usec = 0;
setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
```

### TCP_NODELAY - Disable Nagle algorithm

```c
int flag = 1;
setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
```

**Mục đích:** Gửi dữ liệu ngay lập tức, không đợi buffer đầy

### SO_KEEPALIVE - Enable TCP keepalive

```c
int opt = 1;
setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
```

## Struct sockaddr_in

```c
struct sockaddr_in {
    sa_family_t    sin_family;  /* AF_INET */
    in_port_t      sin_port;    /* Port number (network byte order) */
    struct in_addr sin_addr;    /* IPv4 address */
    char           sin_zero[8]; /* Padding */
};
```

**Chuyển đổi byte order:**
- `htons()` - Host TO Network Short (port)
- `htonl()` - Host TO Network Long (IP)
- `ntohs()` - Network TO Host Short
- `ntohl()` - Network TO Host Long

**Chuyển đổi IP:**
- `inet_pton()` - Presentation TO Network (string -> binary)
- `inet_ntop()` - Network TO Presentation (binary -> string)
- `inet_ntoa()` - Network TO ASCII (deprecated, dùng inet_ntop)

## So sánh TCP vs UDP

| Tiêu chí | TCP | UDP |
|----------|-----|-----|
| Connection | Connection-oriented | Connectionless |
| Độ tin cậy | Đảm bảo delivery | Không đảm bảo |
| Thứ tự | Đảm bảo thứ tự | Không đảm bảo |
| Tốc độ | Chậm hơn | Nhanh hơn |
| Overhead | Cao (header lớn) | Thấp (header nhỏ) |
| Flow control | Có | Không |
| Congestion control | Có | Không |
| Use cases | HTTP, FTP, SSH, Email | DNS, Video streaming, Gaming |

## Debugging Tools

### 1. netstat - Xem trạng thái kết nối

```bash
# Xem tất cả listening ports
netstat -tuln

# Xem các kết nối TCP
netstat -tan

# Xem process sử dụng port
sudo netstat -tulpn | grep :8080
```

### 2. ss - Socket statistics (thay thế netstat)

```bash
# Xem listening sockets
ss -tuln

# Xem TCP connections
ss -tan

# Xem process info
ss -tulpn | grep :8080
```

### 3. lsof - List open files

```bash
# Xem process sử dụng port
sudo lsof -i :8080

# Xem tất cả network connections của process
lsof -p <PID> -a -i
```

### 4. tcpdump - Capture network traffic

```bash
# Capture trên port 8080
sudo tcpdump -i any port 8080 -A

# Capture TCP traffic
sudo tcpdump -i any tcp port 8080 -vv

# Save to file
sudo tcpdump -i any port 8080 -w capture.pcap
```

### 5. nc (netcat) - Network Swiss Army knife

```bash
# Client mode
nc localhost 8080

# Listen mode (server)
nc -l 8080

# UDP mode
nc -u localhost 8081

# Send file
nc localhost 8080 < file.txt
```

### 6. telnet - Test TCP connection

```bash
telnet localhost 8080
```

## Performance Tips

### 1. Reuse address
```c
int opt = 1;
setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```

### 2. Set buffer size
```c
int buffer_size = 64 * 1024; // 64KB
setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size));
setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size));
```

### 3. Disable Nagle for low latency
```c
int flag = 1;
setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
```

### 4. Use I/O multiplexing cho nhiều connections
- select() - max 1024 fds
- poll() - không giới hạn fds
- epoll() - hiệu quả nhất trên Linux

### 5. Thread pool thay vì tạo thread mới mỗi connection

## Tài liệu tham khảo

### Man pages
- `man 2 socket` - socket system call
- `man 2 bind` - bind socket
- `man 2 listen` - listen for connections
- `man 2 accept` - accept connection
- `man 2 connect` - connect to server
- `man 2 send` - send data
- `man 2 recv` - receive data
- `man 2 sendto` - send datagram
- `man 2 recvfrom` - receive datagram
- `man 2 setsockopt` - set socket options
- `man 7 socket` - socket overview
- `man 7 tcp` - TCP protocol
- `man 7 udp` - UDP protocol
- `man 7 ip` - IP protocol

### Sách
- "Unix Network Programming" - W. Richard Stevens
- "TCP/IP Illustrated" - W. Richard Stevens
- "Linux System Programming" - Robert Love
- "The Linux Programming Interface" - Michael Kerrisk

### Online Resources
- Beej's Guide to Network Programming
- Linux man pages: https://man7.org/linux/man-pages/
- RFC 793 (TCP), RFC 768 (UDP)

## Bài tập nâng cao

1. **Thêm tính năng broadcast vào UDP server**
   - Gửi message đến tất cả clients đã biết

2. **Implement chat server**
   - Multi-threaded
   - Broadcast message từ 1 client đến tất cả

3. **Thêm authentication**
   - Yêu cầu username/password khi connect

4. **File transfer**
   - Client upload file lên server
   - Server download file về client

5. **HTTP server đơn giản**
   - Parse HTTP request
   - Serve static files
   - Return HTTP response

6. **Port scanning tool**
   - Scan range of ports
   - Detect open ports

7. **Proxy server**
   - Forward traffic giữa client và server
   - Log traffic

8. **Thêm SSL/TLS encryption**
   - Sử dụng OpenSSL library
   - Secure communication


