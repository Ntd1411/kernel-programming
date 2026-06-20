# Network Programming - Kernel Modules

Các ví dụ về lập trình mạng kernel-space trong Linux, dựa trên slide bài giảng "Lập trình nhân Linux - Phần 2".

## Tổng quan

Phần này tập trung vào kernel network subsystem của Linux, bao gồm:

- Network device driver (loopback driver)
- sk_buff structure và manipulation
- Netfilter hooks để intercept packets
- HTTP protocol analysis trong kernel
- TCP/UDP packet steganography

## Liên quan đến Slide bài giảng

Các module trong phần này đáp ứng yêu cầu từ slide:

| Slide | Nội dung | Module/File tương ứng |
| ------- | ---------- | ---------------------- |
| Slide 4 | Network interface, IP/MAC | network_interface.c |
| Slide 5 | Config interface | network_interface.c |
| Slide 6 | Socket | loopback_driver.c |
| Slide 7 | struct sk_buff | skbuff_demo.c |
| Slide 8 | Luồng gói tin | skbuff_demo.c, loopback_driver.c |
| Slide 9 | Sending flow | loopback_driver.c |
| Slide 10-11 | NAPI, receiving | loopback_driver.c |
| Slide 13.1 | Loopback driver | loopback_driver.c |
| Slide 13.2 | HTTP password detect | http_password_detector.c |
| Bonus | Steganography | tcp_steganography.c, stego_reader.c |

## Cấu trúc thư mục

```
network-programming/
├── loopback_driver.c           # Kernel: Loopback network driver
├── http_password_detector.c    # Kernel: HTTP password detector
├── skbuff_demo.c               # Kernel: sk_buff demo
├── tcp_steganography.c         # Kernel: TCP/UDP steganography
├── network_interface.c         # User: Network interface tool
├── stego_reader.c              # User: Steganography reader
├── test_steganography.sh       # Script: Auto test
├── Makefile                    # Build system
└── README.md                   # Documentation này
```

## Build

### Build tất cả

```bash
make          # Build cả kernel modules và user programs
```

### Build riêng lẻ

```bash
make modules  # Chỉ build kernel modules
make user     # Chỉ build user programs
```

### Clean

```bash
make clean    # Xóa tất cả file đã build
```

## Kernel Modules

### 1. loopback_driver.ko

**Mô tả**: Loopback network driver demo (Bài tập slide 13.1)

**Chức năng:**

- Tạo virtual network interface có tên `myloop0`
- Hỗ trợ up/down interface qua `ip` command
- Gửi/nhận packet qua socket programming
- Demo sk_buff lifecycle trong kernel
- Echo packet ngược lại (loopback behavior)

**Kiến thức demo:**

- Đăng ký network device với `register_netdev()`
- Implement network device operations (`ndo_open`, `ndo_stop`, `ndo_start_xmit`)
- Xử lý transmit: phân tích và log packet info
- Đưa packet vào receive queue với `netif_rx()`
- Quản lý transmission queue với `netif_start_queue()` / `netif_stop_queue()`
- Thống kê packets (tx_packets, tx_bytes, rx_packets, rx_bytes)

**Load module:**

```bash
sudo insmod loopback_driver.ko
```

**Cấu hình interface:**

```bash
sudo ip link set myloop0 up
sudo ip addr add 10.0.0.1/24 dev myloop0
ip addr show myloop0
```

**Test hoạt động:**

```bash
# Ping (packet sẽ loop back)
ping -c 5 10.0.0.2

# Xem log kernel
dmesg | tail -20
```

**Unload module:**

```bash
sudo rmmod loopback_driver
```

**Output mẫu:**

```
[  123.456] === Loopback Network Driver ===
[  123.457] Đang load module...
[  123.458] Khởi tạo loopback device: myloop0
[  123.459] myloop0: MAC address: 52:54:00:12:34:56
[  123.460] Đã tạo network interface: myloop0
```

### 2. http_password_detector.ko

**Mô tả**: HTTP password detector module (Bài tập slide 13.2)

**Chức năng:**

- Sử dụng Netfilter hook để bắt outgoing packets
- Phân tích HTTP POST request
- Tìm chuỗi "password=" trong HTTP body
- Log password ra kernel log
- Lưu password vào file `/var/log/http_passwords.log`

**Kiến thức demo:**

- Netfilter framework: `nf_register_net_hook()`
- Hook point: `NF_INET_POST_ROUTING`
- Phân tích IP header với `ip_hdr()`
- Phân tích TCP header với `tcp_hdr()`
- Detect HTTP protocol từ port và content
- Parse HTTP form data (application/x-www-form-urlencoded)
- Kernel file I/O: `filp_open()`, `kernel_write()`, `filp_close()`

**Load module:**

```bash
sudo insmod http_password_detector.ko
```

**Test với curl:**

```bash
# Test 1: Simple POST
curl -X POST -d "username=admin&password=secret123" http://httpbin.org/post

# Test 2: Multiple fields
curl -X POST -d "user=john&password=mypass456&email=test@example.com" http://example.com

# Test 3: Local server
python3 -m http.server 8000 &
curl -X POST -d "password=test123" http://localhost:8000
```

**Xem kết quả:**

```bash
# Xem kernel log
dmesg | grep -i password

# Xem log file realtime
sudo tail -f /var/log/http_passwords.log

# Đọc toàn bộ log file
sudo cat /var/log/http_passwords.log
```

**Unload module:**

```bash
sudo rmmod http_password_detector
```

**Output mẫu:**

```
[  234.567] HTTP Password Detected!
[  234.568]   Source: 192.168.1.100:54321
[  234.569]   Dest: 93.184.216.34:80
[  234.570]   Password: secret123
```

**Lưu ý an toàn:**

- Module này chỉ dùng cho mục đích học tập
- Không sử dụng trong môi trường production
- Có thể vi phạm privacy nếu dùng sai mục đích
- Chỉ bắt HTTP (không bắt HTTPS vì đã mã hóa)

### 3. skbuff_demo.ko

**Mô tả**: Demo sk_buff structure và manipulation (Slide 7-8)

**Chức năng:**

- Demo 1: Cấp phát và phân tích sk_buff structure
- Demo 2: Thêm data vào sk_buff với `skb_put()` và `skb_reserve()`
- Demo 3: Push/Pull headers (Ethernet, IP, TCP)
- Demo 4: Clone và copy sk_buff
- Demo 5: Phân tích packet structure hoàn chỉnh
- Demo 6: Linear vs Non-linear sk_buff

**Kiến thức demo:**

- Cấu trúc sk_buff: head, data, tail, end pointers
- `alloc_skb()` - Cấp phát sk_buff
- `kfree_skb()` - Giải phóng sk_buff
- `skb_put()` - Thêm data vào tail
- `skb_push()` - Thêm header
- `skb_pull()` - Xóa header
- `skb_reserve()` - Reserve space cho headers
- `skb_clone()` - Clone (shared data)
- `skb_copy()` - Copy (independent data)
- Network/transport header pointers
- Checksum calculation

**Load module:**

```bash
sudo insmod skbuff_demo.ko
```

**Xem kết quả:**

```bash
# Module tự động chạy tất cả demos khi load
dmesg | tail -100

# Hoặc với màu sắc
dmesg -T | tail -100 | grep -E 'Demo|sk_buff|===|Packet'
```

**Unload module:**

```bash
sudo rmmod skbuff_demo
```

**Output mẫu:**

```
[  345.678] ========================================
[  345.679] sk_buff Demo Module
[  345.680] Slide 7: struct skb - quản lý gói tin
[  345.681] ========================================

[  345.682] === Demo 1: Tạo sk_buff ===
[  345.683] sk_buff đã được tạo:
[  345.684]   head: ffff888100000000
[  345.685]   data: ffff888100000000
[  345.686]   tail: ffff888100000000
[  345.687]   end:  ffff8881000005dc
[  345.688]   len:  0 bytes
[  345.689]   truesize: 1664 bytes
```

### 4. tcp_steganography.ko

**Mô tả**: TCP/UDP packet steganography module (Ẩn tin trong gói tin)

**Chức năng:**

- Sử dụng Netfilter hook ở POST_ROUTING (trước khi đến driver)
- Ẩn message vào 3 vị trí trong packet:
  1. IP ID field (16 bits) - Cho cả TCP và UDP
  2. TCP sequence number (8 bits thấp) - Chỉ TCP
  3. UDP checksum (8 bits thấp) - Chỉ UDP
- Đánh dấu packets có hidden message bằng magic marker (0xAB)
- Hỗ trợ custom message qua module parameter
- Tự động encode và tính lại checksum

**Kiến thức demo:**

- Chỉnh sửa sk_buff trước khi gửi xuống driver
- Netfilter hook ở NF_INET_POST_ROUTING
- Encode/decode data vào packet headers
- IP checksum recalculation
- TCP sequence number manipulation
- Module parameters với `module_param_string()`
- Steganography techniques trong network

**Load module với default message:**

```bash
sudo insmod tcp_steganography.ko
```

**Load module với custom message:**

```bash
sudo insmod tcp_steganography.ko message="SECRET_DATA_2024"
```

**Tạo traffic để ẩn tin:**

```bash
# Ping (ẩn vào ICMP packets qua IP ID)
ping -c 20 google.com

# HTTP request (ẩn vào TCP packets)
curl http://example.com

# DNS query (ẩn vào UDP packets)
nslookup google.com

# Download file
wget http://httpbin.org/get
```

**Xem log:**

```bash
# Xem packets đã xử lý
dmesg | grep -i stego

# Xem chi tiết
dmesg | tail -50 | grep -E 'Stego|Ẩn'
```

**Unload module:**

```bash
sudo rmmod tcp_steganography
```

**Output mẫu:**

```
[  456.789] === TCP/UDP Steganography Module ===
[  456.790] Đang load module...
[  456.791] Hidden message: "SECRET_DATA_2024" (17 bytes)
[  456.792] Netfilter hook đã được đăng ký
[  456.793] Module sẵn sàng. Đang ẩn message vào TCP/UDP packets...

[  457.001] Stego: Ẩn 'S' (0x53) vào TCP packet, index=0
[  457.123] Stego: Ẩn 'E' (0x45) vào TCP packet, index=1
[  457.245] Stego: Ẩn 'C' (0x43) vào TCP packet, index=2
```

## User-space Programs

### 1. network_interface

**Mô tả**: Network interface management tool (Slide 4-5)

**Chức năng:**

- Liệt kê tất cả network interfaces
- Hiển thị thông tin chi tiết: IP, MAC, netmask, broadcast, MTU
- Set IP address (cần root)
- Set netmask (cần root)
- Bật/tắt interface - UP/DOWN (cần root)
- Demo system() commands với `ip` command

**Build:**

```bash
make user
```

**Chạy (read-only mode):**

```bash
./network_interface
```

**Chạy với quyền root (full features):**

```bash
sudo ./network_interface
```

**Menu options:**

```
1. Liệt kê tất cả interfaces
2. Hiển thị thông tin interface cụ thể
3. Set interface UP
4. Set interface DOWN
5. Set IP address
6. Set netmask
7. Demo system() commands
0. Thoát
```

**Ví dụ sử dụng:**

```bash
# Xem tất cả interfaces
./network_interface
# Chọn option 1

# Xem chi tiết eth0
./network_interface
# Chọn option 2, nhập: eth0

# Set IP (cần root)
sudo ./network_interface
# Chọn option 5
# Nhập interface: eth0
# Nhập IP: 192.168.1.100
```

**Output mẫu:**

```
=== Thông tin Interface: eth0 ===

Trạng thái: UP
MAC Address: 52:54:00:12:34:56
IP Address: 192.168.1.100
Netmask: 255.255.255.0
Broadcast: 192.168.1.255
MTU: 1500
```

### 2. stego_reader

**Mô tả**: Steganography reader - Đọc tin đã ẩn trong packets

**Chức năng:**

- Bắt TCP/UDP packets với raw socket (AF_PACKET)
- Giải mã hidden message từ 3 nguồn:
  1. IP ID field
  2. TCP sequence number
  3. UDP checksum
- Kiểm tra magic marker để verify packets
- Hiển thị message dạng ASCII và HEX
- Thống kê số bytes đã thu thập
- Lọc chỉ hiển thị message có ý nghĩa (>50% printable chars)

**Build:**

```bash
make user
```

**Chạy:**

```bash
sudo ./stego_reader <interface>
```

**Ví dụ:**

```bash
# Bắt trên Ethernet
sudo ./stego_reader eth0

# Bắt trên WiFi
sudo ./stego_reader wlan0

# Bắt trên loopback
sudo ./stego_reader lo
```

**Test kết hợp với tcp_steganography:**

Terminal 1 - Load module ẩn tin:

```bash
sudo insmod tcp_steganography.ko message="HELLO_WORLD"
```

Terminal 2 - Chạy reader:

```bash
sudo ./stego_reader eth0
```

Terminal 3 - Tạo traffic:

```bash
ping -c 50 8.8.8.8
curl http://example.com
wget http://httpbin.org/get
```

**Output mẫu:**

```
========================================
Steganography Reader
========================================
Interface: eth0
Đang bắt packets và tìm hidden messages...
Nhấn Ctrl+C để dừng

[1] IP ID: Tìm thấy byte 0x48 'H'
[2] IP ID: Tìm thấy byte 0x45 'E'
[3] IP ID: Tìm thấy byte 0x4C 'L'
[4] IP ID: Tìm thấy byte 0x4C 'L'
[5] IP ID: Tìm thấy byte 0x4F 'O'

[IP ID] Hidden Message (11 bytes):
  ASCII: "HELLO_WORLD"
  HEX: 48 45 4C 4C 4F 5F 57 4F 52 4C 44
```

**Dừng chương trình:**

```bash
# Nhấn Ctrl+C để dừng và xem thống kê cuối
```

## Scripts tiện ích

### test_steganography.sh

**Mô tả**: Script tự động test steganography module

**Chức năng:**

- Tự động build modules và programs
- Load tcp_steganography module
- Chạy stego_reader trong background
- Tạo network traffic (ping, curl)
- Thu thập và hiển thị kết quả
- Cleanup tự động

**Chạy:**

```bash
sudo ./test_steganography.sh
```

**Script sẽ thực hiện:**

1. Kiểm tra quyền root
2. Kiểm tra interface tồn tại
3. Build nếu chưa có
4. Load module với message "HELLO_SECRET_WORLD"
5. Chạy stego_reader trong background
6. Tạo traffic trong 15 giây
7. Hiển thị kết quả từ reader và dmesg
8. Unload module và cleanup

**Cấu hình trong script:**

```bash
INTERFACE="eth0"              # Interface để capture
MESSAGE="HELLO_SECRET_WORLD"  # Message để ẩn
TEST_DURATION=15              # Thời gian test (giây)
```

**Output mẫu:**

```
==========================================
TCP/UDP Steganography Test Script
==========================================

Cấu hình:
  Interface: eth0
  Message: HELLO_SECRET_WORLD
  Test duration: 15s

[1] Load tcp_steganography module...
  Module loaded với message: HELLO_SECRET_WORLD

[2] Khởi động stego_reader trong background...
  Reader PID: 12345
  Log file: /tmp/stego_reader.log

[3] Tạo network traffic...
  Ping google.com...
  Curl example.com...
  Đợi 15 giây để thu thập data...

[4] Dừng stego_reader...

==========================================
KẾT QUẢ
==========================================

Log từ stego_reader:
--------------------
[IP ID] Hidden Message (19 bytes):
  ASCII: "HELLO_SECRET_WORLD"
  HEX: 48 45 4C 4C 4F 5F 53 45 43 52 45 54 5F 57 4F 52 4C 44

[5] Unload module...
  Module unloaded

Test hoàn thành!
```

## System Calls và Kernel APIs

### Network Device Operations

```c
// Đăng ký/hủy network device
int register_netdev(struct net_device *dev);
void unregister_netdev(struct net_device *dev);

// Cấp phát network device
struct net_device *alloc_netdev(int sizeof_priv, 
                                const char *name,
                                unsigned char name_assign_type,
                                void (*setup)(struct net_device *));

void free_netdev(struct net_device *dev);

// Network device operations
struct net_device_ops {
    int (*ndo_open)(struct net_device *dev);
    int (*ndo_stop)(struct net_device *dev);
    netdev_tx_t (*ndo_start_xmit)(struct sk_buff *skb, 
                                   struct net_device *dev);
    struct net_device_stats* (*ndo_get_stats)(struct net_device *dev);
};

// Queue management
void netif_start_queue(struct net_device *dev);
void netif_stop_queue(struct net_device *dev);
```

### sk_buff Operations

```c
// Cấp phát/giải phóng
struct sk_buff *alloc_skb(unsigned int size, gfp_t priority);
void kfree_skb(struct sk_buff *skb);

// Thêm/xóa data
unsigned char *skb_put(struct sk_buff *skb, unsigned int len);
unsigned char *skb_push(struct sk_buff *skb, unsigned int len);
unsigned char *skb_pull(struct sk_buff *skb, unsigned int len);
void skb_reserve(struct sk_buff *skb, int len);

// Clone/copy
struct sk_buff *skb_clone(struct sk_buff *skb, gfp_t priority);
struct sk_buff *skb_copy(const struct sk_buff *skb, gfp_t priority);

// Header pointers
void skb_reset_mac_header(struct sk_buff *skb);
void skb_set_network_header(struct sk_buff *skb, int offset);
void skb_set_transport_header(struct sk_buff *skb, int offset);

struct ethhdr *eth_hdr(const struct sk_buff *skb);
struct iphdr *ip_hdr(const struct sk_buff *skb);
struct tcphdr *tcp_hdr(const struct sk_buff *skb);
struct udphdr *udp_hdr(const struct sk_buff *skb);

// Gửi/nhận
int netif_rx(struct sk_buff *skb);
```

### Netfilter Hooks

```c
// Đăng ký/hủy hook
int nf_register_net_hook(struct net *net, const struct nf_hook_ops *ops);
void nf_unregister_net_hook(struct net *net, const struct nf_hook_ops *ops);

// Hook function signature
unsigned int hook_func(void *priv,
                       struct sk_buff *skb,
                       const struct nf_hook_state *state);

// Hook points
NF_INET_PRE_ROUTING    // Sau khi nhận, trước routing decision
NF_INET_LOCAL_IN       // Packets đến local process
NF_INET_FORWARD        // Packets được forward
NF_INET_LOCAL_OUT      // Packets từ local process
NF_INET_POST_ROUTING   // Sau routing, trước driver (dùng để ẩn tin)

// Return values
NF_ACCEPT   // Chấp nhận packet, tiếp tục xử lý
NF_DROP     // Drop packet, không tiếp tục
NF_STOLEN   // Đã xử lý xong, kernel không cần làm gì thêm
NF_QUEUE    // Đưa vào queue để xử lý ở user-space
NF_REPEAT   // Gọi lại hook này
```

### Network Interface ioctl

```c
// Socket cho ioctl
int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

// Get/Set interface info
int ioctl(int sockfd, unsigned long request, struct ifreq *ifr);

// Các request codes
SIOCGIFADDR      // Get IP address
SIOCSIFADDR      // Set IP address
SIOCGIFHWADDR    // Get MAC address (hardware address)
SIOCGIFNETMASK   // Get netmask
SIOCSIFNETMASK   // Set netmask
SIOCGIFBRDADDR   // Get broadcast address
SIOCGIFFLAGS     // Get interface flags (UP/DOWN/etc)
SIOCSIFFLAGS     // Set interface flags
SIOCGIFMTU       // Get MTU (Maximum Transmission Unit)
SIOCSIFMTU       // Set MTU

// Cấu trúc ifreq
struct ifreq {
    char ifr_name[IFNAMSIZ];  // Interface name (e.g., "eth0")
    union {
        struct sockaddr ifr_addr;      // Address
        struct sockaddr ifr_netmask;   // Netmask
        struct sockaddr ifr_hwaddr;    // Hardware address
        short ifr_flags;               // Flags
        int ifr_mtu;                   // MTU
    };
};
```

### Steganography APIs

```c
// Chỉnh sửa IP header
struct iphdr *iph = ip_hdr(skb);
iph->id = htons(encoded_value);

// Tính lại IP checksum
void recalculate_ip_checksum(struct iphdr *iph) {
    iph->check = 0;
    // Tính checksum...
    iph->check = ~sum;
}

// Chỉnh sửa TCP header
struct tcphdr *tcph = tcp_hdr(skb);
tcph->seq = htonl(new_seq);
tcph->res1 = marker_bits;  // Reserved bits

// Chỉnh sửa UDP header
struct udphdr *udph = udp_hdr(skb);
udph->check = htons(new_checksum);

// Raw socket capture (user-space)
int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

// Bind to interface
struct sockaddr_ll sll = {
    .sll_family = AF_PACKET,
    .sll_ifindex = if_nametoindex("eth0"),
    .sll_protocol = htons(ETH_P_ALL)
};
bind(sockfd, (struct sockaddr *)&sll, sizeof(sll));

// Receive packets
recvfrom(sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL);
```

## Troubleshooting

### Lỗi: Module không load được

```bash
# Kiểm tra kernel version
uname -r

# Kiểm tra kernel headers đã cài
ls /lib/modules/$(uname -r)/build

# Cài kernel headers (Ubuntu/Debian)
sudo apt-get install linux-headers-$(uname -r)

# Cài kernel headers (CentOS/RHEL)
sudo yum install kernel-devel-$(uname -r)

# Cài kernel headers (Arch Linux)
sudo pacman -S linux-headers

# Kiểm tra lỗi khi load
dmesg | tail -20
```

### Lỗi: Không tạo được network interface

```bash
# Kiểm tra quyền root
sudo -v

# Xem log chi tiết
sudo dmesg | tail -30

# Kiểm tra interface đã tồn tại
ip link show myloop0

# Xóa interface cũ nếu bị treo
sudo ip link delete myloop0

# Load lại module
sudo rmmod loopback_driver
sudo insmod loopback_driver.ko
```

### Lỗi: HTTP detector không bắt được packet

```bash
# Kiểm tra module đã load
lsmod | grep http_password

# Xem log
dmesg | grep -i http

# Test với local server thay vì external
python3 -m http.server 8000 &
curl -X POST -d "password=test123" http://localhost:8000

# Kiểm tra firewall không chặn
sudo iptables -L -n

# Kiểm tra log file được tạo
ls -l /var/log/http_passwords.log
sudo tail /var/log/http_passwords.log

# Thử với tcpdump để xem có traffic không
sudo tcpdump -i any -n port 80 -A
```

### Lỗi: Stego reader không bắt được packets

```bash
# Kiểm tra interface tồn tại
ip link show

# Liệt kê các interface
ip link show | grep '^[0-9]' | cut -d: -f2

# Kiểm tra quyền root
whoami

# Test với tcpdump trước
sudo tcpdump -i eth0 -c 10

# Thử interface khác
sudo ./stego_reader lo      # Loopback
sudo ./stego_reader wlan0   # WiFi
sudo ./stego_reader enp0s3  # Ethernet (tên khác)

# Kiểm tra có packets đang đi qua không
sudo tcpdump -i eth0 -c 5 -n
```

### Lỗi: Không giải mã được message

```bash
# Kiểm tra module đã load
lsmod | grep tcp_steganography

# Kiểm tra message trong module
dmesg | grep "Hidden message"

# Xem log kernel realtime
sudo dmesg -w

# Test đơn giản với ping
ping -c 20 8.8.8.8

# Xem raw packet để debug
sudo tcpdump -i eth0 -X -c 5

# Kiểm tra IP ID field
sudo tcpdump -i eth0 -v -c 10 | grep -i "id"
```

### Lỗi: Build failed

```bash
# Xem lỗi chi tiết
make 2>&1 | tee build.log

# Clean và build lại
make clean
make

# Kiểm tra compiler
gcc --version

# Kiểm tra module symbols
nm loopback_driver.ko | grep init

# Kiểm tra dependencies
modinfo loopback_driver.ko
```

## Debug và Monitoring

### Xem kernel log

```bash
# Xem toàn bộ log
dmesg

# Xem log mới nhất
dmesg | tail -50

# Xem log realtime
dmesg -w

# Xem log với timestamp
dmesg -T

# Filter theo module
dmesg | grep -E 'loopback|stego|http_password'

# Clear log
sudo dmesg -C
```

### Kiểm tra modules đã load

```bash
# Liệt kê tất cả modules
lsmod

# Tìm module cụ thể
lsmod | grep loopback
lsmod | grep tcp_steganography

# Xem thông tin module
modinfo loopback_driver.ko
modinfo tcp_steganography.ko

# Xem module parameters
cat /sys/module/tcp_steganography/parameters/message
```

### Kiểm tra network

```bash
# Xem tất cả interfaces
ip link show
ifconfig -a

# Xem routing table
ip route show
route -n

# Xem thống kê interface
ip -s link show eth0
ifconfig eth0

# Xem connections
ss -tunap
netstat -tunap

# Capture packets
sudo tcpdump -i eth0 -w capture.pcap
sudo tcpdump -i eth0 -X -c 10
```

## Ứng dụng thực tế

### 1. Loopback Network Driver

- **Học tập**: Hiểu cách hoạt động của network device driver
- **Testing**: Tạo virtual network cho testing mà không cần hardware
- **Development**: Debug network stack issues
- **Simulation**: Giả lập network devices cho lab/training

### 2. HTTP Password Detector

- **Security**: Network security monitoring và audit
- **IDS/IPS**: Intrusion Detection/Prevention Systems
- **Compliance**: Kiểm tra password được gửi qua plain text
- **Forensics**: Network forensics và incident response

### 3. sk_buff Demo

- **Learning**: Học kernel network stack architecture
- **Debugging**: Debug packet processing issues
- **Optimization**: Hiểu performance bottlenecks
- **Development**: Phát triển custom protocol handlers

### 4. TCP Steganography

- **Security Research**: Nghiên cứu covert channels
- **Red Team**: Data exfiltration techniques
- **Blue Team**: Detection và prevention methods
- **Forensics**: Network steganography analysis

## Lưu ý quan trọng

### Quyền truy cập

**Kernel modules:**

- Tất cả kernel modules cần quyền root để load/unload
- Sử dụng `sudo insmod` và `sudo rmmod`

**User programs:**

- `network_interface`: Chỉ đọc không cần root, ghi cần root
- `stego_reader`: Luôn cần root (raw socket)

### An toàn

**HTTP Password Detector:**

- Chỉ dùng cho mục đích học tập và testing
- Không deploy trong môi trường production
- Có thể vi phạm privacy laws nếu dùng sai

**TCP Steganography:**

- Demo kỹ thuật, không dùng cho mục đích xấu
- Có thể bị firewall/IDS detect
- Ảnh hưởng đến network performance
- Một số ISP có thể block modified packets

### Performance

**Netfilter hooks:**

- Hook ở POST_ROUTING có overhead thấp nhất
- Tránh xử lý phức tạp trong hook function
- Sử dụng `NF_ACCEPT` để không ảnh hưởng traffic

**sk_buff manipulation:**

- Luôn kiểm tra `skb_is_nonlinear()` trước khi modify
- Gọi `skb_linearize()` nếu cần
- Tính lại checksum sau khi modify headers

## Tài liệu tham khảo

### Linux Kernel Documentation

- `Documentation/networking/` - Network subsystem docs
- `Documentation/networking/netdevices.txt` - Network device drivers
- `Documentation/networking/netfilter-hacking.txt` - Netfilter internals

### Header files quan trọng

- `include/linux/skbuff.h` - sk_buff structure và APIs
- `include/linux/netdevice.h` - Network device APIs
- `include/linux/netfilter.h` - Netfilter framework
- `include/linux/ip.h` - IP header definitions
- `include/linux/tcp.h` - TCP header definitions
- `include/linux/udp.h` - UDP header definitions
- `include/linux/if_ether.h` - Ethernet definitions

### Books và Resources

- "Linux Kernel Networking" by Rami Rosen
- "Understanding Linux Network Internals" by Christian Benvenuti
- "Linux Device Drivers" by Jonathan Corbet (Chapter 17: Network Drivers)
- Linux kernel source code: `net/core/`, `net/ipv4/`, `drivers/net/`

### Online Resources

- <https://www.kernel.org/doc/html/latest/networking/>
- <https://wiki.linuxfoundation.org/networking/start>
- <https://www.kernel.org/doc/Documentation/networking/>

## Tổng kết

### Thống kê dự án

| Loại | Số lượng | Dòng code | Mô tả |
|------|----------|-----------|-------|
| Kernel Modules | 4 | ~1000 | loopback, http_detector, skbuff_demo, steganography |
| User Programs | 2 | ~850 | network_interface, stego_reader |
| Scripts | 1 | ~100 | test_steganography.sh |
| Documentation | 1 | - | README.md (file này) |
| **Tổng cộng** | **8** | **~1950** | **Complete network programming suite** |

### Độ phủ nội dung slide

Repository này đáp ứng ~95% nội dung slide bài giảng:

- ✓ Slide 4-6: Network interface, socket (network_interface.c)
- ✓ Slide 7: struct sk_buff (skbuff_demo.c)
- ✓ Slide 8-9: Luồng gói tin, sending flow (loopback_driver.c, skbuff_demo.c)
- ✓ Slide 10-11: NAPI, receiving flow (loopback_driver.c)
- ✓ Slide 13.1: Loopback driver (loopback_driver.c)
- ✓ Slide 13.2: HTTP password detect (http_password_detector.c)
- ✓ Bonus: Steganography (tcp_steganography.c, stego_reader.c)

### Kiến thức đạt được

Sau khi học xong phần này, bạn sẽ hiểu:

1. **Network Device Driver**: Cách viết driver cho network interface
2. **sk_buff**: Cấu trúc dữ liệu cốt lõi của network stack
3. **Netfilter**: Framework để intercept và modify packets
4. **Packet Flow**: Luồng đi của packet từ user-space đến driver
5. **Header Manipulation**: Cách thêm/xóa/sửa protocol headers
6. **Network Programming**: Cả kernel-space và user-space

## Quick Start Guide

### Bước 1: Build tất cả

```bash
cd system-programming/network-programming
make
```

### Bước 2: Test từng module

**Test loopback driver:**

```bash
sudo insmod loopback_driver.ko
sudo ip link set myloop0 up
sudo ip addr add 10.0.0.1/24 dev myloop0
ping -c 5 10.0.0.2
sudo rmmod loopback_driver
```

**Test HTTP detector:**

```bash
sudo insmod http_password_detector.ko
curl -X POST -d "password=test123" http://httpbin.org/post
dmesg | grep password
sudo rmmod http_password_detector
```

**Test sk_buff demo:**

```bash
sudo insmod skbuff_demo.ko
dmesg | tail -100
sudo rmmod skbuff_demo
```

**Test steganography:**

```bash
# Tự động
sudo ./test_steganography.sh

# Hoặc thủ công
sudo insmod tcp_steganography.ko message="SECRET"
sudo ./stego_reader eth0 &
ping -c 20 google.com
sudo rmmod tcp_steganography
```

### Bước 3: Xem kết quả

```bash
# Xem kernel log
dmesg | tail -50

# Xem network interfaces
ip link show

# Xem file log
sudo cat /var/log/http_passwords.log
```

## FAQ - Câu hỏi thường gặp

**Q: Module load bị lỗi "Invalid module format"?**
A: Module được compile cho kernel version khác. Chạy `make clean` và `make` lại.

**Q: Không thấy interface myloop0 sau khi load?**
A: Xem `dmesg | tail` để kiểm tra lỗi. Có thể cần unload module cũ trước.

**Q: HTTP detector không bắt được password?**
A: Chỉ bắt được HTTP (port 80), không bắt HTTPS. Thử với local server.

**Q: Stego reader không hiển thị gì?**
A: Đảm bảo tcp_steganography module đã load và có traffic đi qua interface.

**Q: Làm sao để thay đổi message trong steganography?**
A: Load module với parameter: `sudo insmod tcp_steganography.ko message="YOUR_MESSAGE"`

**Q: Có thể dùng cho production không?**
A: KHÔNG. Đây là code demo cho mục đích học tập, chưa optimize và test đầy đủ.

**Q: Code có hoạt động trên kernel version X?**
A: Được test trên kernel 5.x và 6.x. Các version cũ hơn có thể cần chỉnh sửa APIs.

**Q: Làm sao để contribute?**
A: Fork repo, tạo branch mới, commit changes, và tạo pull request.

## Giấy phép và Trách nhiệm

### Giấy phép

Tất cả code trong repository này được phát hành dưới giấy phép GPL v2, tương thích với Linux kernel.

### Trách nhiệm

- Code được cung cấp "AS IS" không có bảo hành
- Chỉ dùng cho mục đích học tập và nghiên cứu
- Không sử dụng cho mục đích bất hợp pháp
- Tác giả không chịu trách nhiệm cho bất kỳ thiệt hại nào

### An toàn

- Luôn test trong môi trường sandbox/VM trước
- Không load modules không rõ nguồn gốc
- Backup dữ liệu quan trọng trước khi test
- Không test trên production systems

## Học viện Kỹ thuật Mật mã

**Dự án**: Lập trình nhân Linux - Phần 2: Network Programming  
**Nguồn**: Slide bài giảng "Lập trình nhân Linux"  
**Mục đích**: Học tập và nghiên cứu  
**Năm**: 2024-2026

### Bài tập đã hoàn thành

- [x] Slide 13.1: Viết loopback network driver
- [x] Slide 13.2: HTTP password detector với Netfilter
- [x] Bonus: sk_buff structure demo
- [x] Bonus: TCP/UDP steganography

### Các module bổ sung

Ngoài yêu cầu của slide, repository còn bổ sung:

- User-space network interface management tool
- Steganography reader program
- Automated test scripts
- Comprehensive documentation

## Liên hệ và đóng góp

### Báo lỗi (Issues)

Nếu phát hiện lỗi hoặc có đề xuất, vui lòng:

1. Kiểm tra đã có issue tương tự chưa
2. Cung cấp thông tin: kernel version, distro, lỗi cụ thể
3. Đính kèm output của `dmesg` nếu có

### Đóng góp code (Pull Requests)

Chào đón mọi đóng góp:

- Bug fixes
- New features
- Documentation improvements
- Code optimization
- Test cases

### Hướng dẫn đóng góp

1. Fork repository
2. Tạo branch: `git checkout -b feature/ten-tinh-nang`
3. Commit changes: `git commit -am 'Thêm tính năng X'`
4. Push: `git push origin feature/ten-tinh-nang`
5. Tạo Pull Request

## Changelog

### Version 1.0 (2024-06)

- Initial release
- 4 kernel modules
- 2 user-space programs
- Complete documentation
- Test scripts

## Acknowledgments

Cảm ơn:

- Học viện Kỹ thuật Mật mã - Slide bài giảng
- Linux kernel community - Documentation và source code
- Stack Overflow community - Giải đáp thắc mắc
- GitHub community - Tools và inspiration

---

**Lưu ý cuối**: Repository này là tài liệu học tập. Hãy đọc kỹ code, hiểu rõ trước khi chạy, và luôn test trong môi trường an toàn.

**Happy Kernel Hacking!** 🐧
