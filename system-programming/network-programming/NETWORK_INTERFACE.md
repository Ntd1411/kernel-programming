# Network Interface Management

Quản lý network interface thông qua ioctl() system calls và ip commands.

## Mô tả

Chương trình `network_interface.c` minh họa cách quản lý network interface ở mức system programming:
- Liệt kê tất cả interfaces
- Đọc thông tin: IP, MAC, netmask, broadcast, MTU
- Set IP address và netmask
- Bật/tắt interface (UP/DOWN)
- Sử dụng system() để gọi ip commands

## Các System Calls Chính

### ioctl() với Network Interface

```c
int ioctl(int sockfd, unsigned long request, ...);
```

Các request codes quan trọng:
- `SIOCGIFADDR` - Lấy IP address
- `SIOCSIFADDR` - Set IP address
- `SIOCGIFHWADDR` - Lấy MAC address
- `SIOCSIFHWADDR` - Set MAC address
- `SIOCGIFNETMASK` - Lấy netmask
- `SIOCSIFNETMASK` - Set netmask
- `SIOCGIFBRDADDR` - Lấy broadcast address
- `SIOCGIFMTU` - Lấy MTU
- `SIOCGIFFLAGS` - Lấy interface flags
- `SIOCSIFFLAGS` - Set interface flags (UP/DOWN)

### getifaddrs()

```c
int getifaddrs(struct ifaddrs **ifap);
void freeifaddrs(struct ifaddrs *ifa);
```

Lấy danh sách tất cả interfaces trong hệ thống.

## Cấu trúc Dữ liệu

### struct ifreq

```c
struct ifreq {
    char ifr_name[IFNAMSIZ];    /* Tên interface (eth0, wlan0, ...) */
    union {
        struct sockaddr ifr_addr;      /* IP address */
        struct sockaddr ifr_hwaddr;    /* MAC address */
        struct sockaddr ifr_netmask;   /* Netmask */
        struct sockaddr ifr_broadaddr; /* Broadcast address */
        short ifr_flags;               /* Interface flags */
        int ifr_mtu;                   /* MTU */
    };
};
```

### struct ifaddrs

```c
struct ifaddrs {
    struct ifaddrs  *ifa_next;    /* Con trỏ tới interface tiếp theo */
    char            *ifa_name;    /* Tên interface */
    unsigned int     ifa_flags;   /* Flags (IFF_UP, IFF_RUNNING, ...) */
    struct sockaddr *ifa_addr;    /* Địa chỉ interface */
    struct sockaddr *ifa_netmask; /* Netmask */
    union {
        struct sockaddr *ifu_broadaddr; /* Broadcast address */
        struct sockaddr *ifu_dstaddr;   /* Destination address (P2P) */
    } ifa_ifu;
    void            *ifa_data;    /* Dữ liệu phụ thuộc address family */
};
```

## Build và Run

### Build

```bash
cd system-programming/network-programming
make network_interface
```

### Chạy

Chế độ read-only (không cần root):
```bash
./network_interface
```

Chế độ đầy đủ (cần root):
```bash
sudo ./network_interface
```

## Các Chức năng

### 1. Liệt kê tất cả interfaces

Hiển thị danh sách interfaces và trạng thái (UP/DOWN):
```
Danh sach network interfaces:
  lo       [UP]
  eth0     [UP]
  wlan0    [DOWN]
```

### 2. Hiển thị thông tin interface

Xem chi tiết một interface cụ thể:
```
=== Thông tin Interface: eth0 ===

Trạng thái: UP
MAC Address: 52:54:00:12:34:56
IP Address: 192.168.1.100
Netmask: 255.255.255.0
Broadcast: 192.168.1.255
MTU: 1500
```

### 3. Set interface UP

Bật interface (cần root):
```
=== Set Interface UP ===
Interface: eth0
Interface eth0 đã được set UP
```

### 4. Set interface DOWN

Tắt interface (cần root):
```
=== Set Interface DOWN ===
Interface: eth0
Interface eth0 đã được set DOWN
```

### 5. Set IP address

Thay đổi IP address (cần root):
```
=== Set IP Address ===
Interface: eth0
IP: 192.168.1.200
Đã set IP 192.168.1.200 cho interface eth0
```

### 6. Set netmask

Thay đổi netmask (cần root):
```
=== Set Netmask ===
Interface: eth0
Netmask: 255.255.255.0
Đã set netmask 255.255.255.0 cho interface eth0
```

### 7. Demo system() commands

Sử dụng ip command để thao tác:
```
=== Demo sử dụng system() commands ===

1. Hiển thị thông tin interface:
Command: ip addr show eth0
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500
    link/ether 52:54:00:12:34:56 brd ff:ff:ff:ff:ff:ff
    inet 192.168.1.100/24 brd 192.168.1.255 scope global eth0

2. Hiển thị routing table:
Command: ip route show
default via 192.168.1.1 dev eth0
192.168.1.0/24 dev eth0 proto kernel scope link src 192.168.1.100

3. Hiển thị statistics:
Command: ip -s link show eth0
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500
    RX: bytes  packets  errors  dropped overrun mcast
    1234567    8901     0       0       0       0
    TX: bytes  packets  errors  dropped carrier collsns
    2345678    9012     0       0       0       0
```

## Interface Flags

Các flags quan trọng trong `ifr_flags`:
- `IFF_UP` - Interface đang UP
- `IFF_RUNNING` - Interface đang hoạt động
- `IFF_BROADCAST` - Hỗ trợ broadcast
- `IFF_LOOPBACK` - Loopback interface
- `IFF_POINTOPOINT` - Point-to-point link
- `IFF_MULTICAST` - Hỗ trợ multicast
- `IFF_PROMISC` - Promiscuous mode

## Ví dụ Code

### Lấy IP address

```c
int get_ip_address(const char *ifname, char *ip_str, size_t len) {
    int sockfd;
    struct ifreq ifr;
    struct sockaddr_in *addr;
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    
    if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
        perror("ioctl SIOCGIFADDR");
        close(sockfd);
        return -1;
    }
    
    addr = (struct sockaddr_in *)&ifr.ifr_addr;
    inet_ntop(AF_INET, &addr->sin_addr, ip_str, len);
    
    close(sockfd);
    return 0;
}
```

### Set IP address

```c
int set_ip_address(const char *ifname, const char *ip) {
    int sockfd;
    struct ifreq ifr;
    struct sockaddr_in *addr;
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    
    addr = (struct sockaddr_in *)&ifr.ifr_addr;
    addr->sin_family = AF_INET;
    
    if (inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {
        fprintf(stderr, "Địa chỉ IP không hợp lệ\n");
        close(sockfd);
        return -1;
    }
    
    if (ioctl(sockfd, SIOCSIFADDR, &ifr) < 0) {
        perror("ioctl SIOCSIFADDR");
        close(sockfd);
        return -1;
    }
    
    close(sockfd);
    return 0;
}
```

### Set interface UP/DOWN

```c
int set_interface_up(const char *ifname) {
    int sockfd;
    struct ifreq ifr;
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    
    /* Lấy flags hiện tại */
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0) {
        perror("ioctl SIOCGIFFLAGS");
        close(sockfd);
        return -1;
    }
    
    /* Set UP flag */
    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0) {
        perror("ioctl SIOCSIFFLAGS");
        close(sockfd);
        return -1;
    }
    
    close(sockfd);
    return 0;
}
```

## So sánh ioctl() vs ip command

### Sử dụng ioctl()

Ưu điểm:
- Kiểm soát trực tiếp từ C code
- Không cần fork process
- Hiệu năng cao hơn
- Có thể kiểm tra return codes chi tiết

Nhược điểm:
- Code phức tạp hơn
- Cần hiểu về socket programming
- Phải xử lý các cấu trúc dữ liệu kernel

### Sử dụng system()/ip command

Ưu điểm:
- Code đơn giản, dễ hiểu
- Dễ debug (có thể test command trực tiếp)
- Tận dụng công cụ có sẵn

Nhược điểm:
- Fork process mỗi lần gọi (overhead)
- Khó kiểm soát errors
- Phụ thuộc vào tool có sẵn trong hệ thống
- Bảo mật kém hơn (command injection risk)

## Lưu ý quan trọng

### Quyền root

Hầu hết thao tác ghi (set IP, UP/DOWN) cần quyền root:
```bash
sudo ./network_interface
```

### Kiểm tra quyền trong code

```c
if (geteuid() != 0) {
    fprintf(stderr, "Cần quyền root!\n");
    fprintf(stderr, "Chạy với: sudo %s\n", argv[0]);
    return 1;
}
```

### Interface names

Các tên interface phổ biến:
- `lo` - Loopback interface (127.0.0.1)
- `eth0`, `eth1` - Ethernet interfaces
- `wlan0`, `wlan1` - Wireless interfaces
- `enp0s3` - Ethernet (systemd naming)
- `wlp2s0` - Wireless (systemd naming)

## Debugging và Testing

### Xem kernel messages

```bash
dmesg | tail
dmesg | grep eth0
```

### Kiểm tra interface status

```bash
ip link show
ip addr show eth0
ip route show
```

### Test script

```bash
./network_interface << EOF
1
2
eth0
0
EOF
```

## Tham khảo

- `man 7 netdevice` - Network device interface
- `man 2 ioctl` - ioctl system call
- `man 3 getifaddrs` - Get interface addresses
- `man 8 ip` - ip command

