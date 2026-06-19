/*
 * network_interface.c - Thao tác với Network Interface
 * 
 * Minh họa: đọc/set IP, MAC address, up/down interface
 * Sử dụng ioctl() system calls
 * 
 * Lưu ý: Cần quyền root để chạy
 * 
 * Biên dịch: gcc -o network_interface network_interface.c
 * Chạy: sudo ./network_interface
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>

#define MAX_INTERFACE_NAME 16

/*
 * Liệt kê tất cả network interfaces
 */
void list_interfaces(void) {
    struct ifaddrs *ifaddr, *ifa;
    int family;
    char host[NI_MAXHOST];
    
    printf("\n=== Danh sách Network Interfaces ===\n\n");
    
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return;
    }
    
    printf("%-15s %-10s %-20s\n", "Interface", "Family", "Address");
    printf("-------------------------------------------------------\n");
    
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;
        
        family = ifa->ifa_addr->sa_family;
        
        if (family == AF_INET || family == AF_INET6) {
            int s = getnameinfo(ifa->ifa_addr,
                               (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                                     sizeof(struct sockaddr_in6),
                               host, NI_MAXHOST,
                               NULL, 0, NI_NUMERICHOST);
            
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                continue;
            }
            
            printf("%-15s %-10s %-20s\n", 
                   ifa->ifa_name,
                   (family == AF_INET) ? "IPv4" : "IPv6",
                   host);
        }
    }
    
    freeifaddrs(ifaddr);
    printf("\n");
}

/*
 * Lấy IP address của interface
 */
int get_ip_address(const char *ifname, char *ip_str, size_t len) {
    int sockfd;
    struct ifreq ifr;
    struct sockaddr_in *ipaddr;
    
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
    
    ipaddr = (struct sockaddr_in *)&ifr.ifr_addr;
    inet_ntop(AF_INET, &ipaddr->sin_addr, ip_str, len);
    
    close(sockfd);
    return 0;
}

/*
 * Lấy MAC address của interface
 */
int get_mac_address(const char *ifname, char *mac_str, size_t len) {
    int sockfd;
    struct ifreq ifr;
    unsigned char *mac;
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("ioctl SIOCGIFHWADDR");
        close(sockfd);
        return -1;
    }
    
    mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;
    snprintf(mac_str, len, "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    close(sockfd);
    return 0;
}

/*
 * Lấy netmask của interface
 */
int get_netmask(const char *ifname, char *mask_str, size_t len) {
    int sockfd;
    struct ifreq ifr;
    struct sockaddr_in *netmask;
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    
    if (ioctl(sockfd, SIOCGIFNETMASK, &ifr) < 0) {
        perror("ioctl SIOCGIFNETMASK");
        close(sockfd);
        return -1;
    }
    
    netmask = (struct sockaddr_in *)&ifr.ifr_netmask;
    inet_ntop(AF_INET, &netmask->sin_addr, mask_str, len);
    
    close(sockfd);
    return 0;
}

/*
 * Lấy broadcast address của interface
 */
int get_broadcast_address(const char *ifname, char *bcast_str, size_t len) {
    int sockfd;
    struct ifreq ifr;
    struct sockaddr_in *broadcast;
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    
    if (ioctl(sockfd, SIOCGIFBRDADDR, &ifr) < 0) {
        perror("ioctl SIOCGIFBRDADDR");
        close(sockfd);
        return -1;
    }
    
    broadcast = (struct sockaddr_in *)&ifr.ifr_broadaddr;
    inet_ntop(AF_INET, &broadcast->sin_addr, bcast_str, len);
    
    close(sockfd);
    return 0;
}

/*
 * Lấy MTU của interface
 */
int get_mtu(const char *ifname, int *mtu) {
    int sockfd;
    struct ifreq ifr;
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    
    if (ioctl(sockfd, SIOCGIFMTU, &ifr) < 0) {
        perror("ioctl SIOCGIFMTU");
        close(sockfd);
        return -1;
    }
    
    *mtu = ifr.ifr_mtu;
    
    close(sockfd);
    return 0;
}

/*
 * Kiểm tra interface có UP không
 */
int is_interface_up(const char *ifname) {
    int sockfd;
    struct ifreq ifr;
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0) {
        perror("ioctl SIOCGIFFLAGS");
        close(sockfd);
        return -1;
    }
    
    close(sockfd);
    return (ifr.ifr_flags & IFF_UP) ? 1 : 0;
}

/*
 * Set interface UP
 */
int set_interface_up(const char *ifname) {
    int sockfd;
    struct ifreq ifr;
    
    printf("\n=== Set Interface UP ===\n");
    printf("Interface: %s\n", ifname);
    
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
    printf("Interface %s đã được set UP\n", ifname);
    return 0;
}

/*
 * Set interface DOWN
 */
int set_interface_down(const char *ifname) {
    int sockfd;
    struct ifreq ifr;
    
    printf("\n=== Set Interface DOWN ===\n");
    printf("Interface: %s\n", ifname);
    
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
    
    /* Clear UP flag */
    ifr.ifr_flags &= ~IFF_UP;
    
    if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0) {
        perror("ioctl SIOCSIFFLAGS");
        close(sockfd);
        return -1;
    }
    
    close(sockfd);
    printf("Interface %s đã được set DOWN\n", ifname);
    return 0;
}

/*
 * Set IP address cho interface
 */
int set_ip_address(const char *ifname, const char *ip) {
    int sockfd;
    struct ifreq ifr;
    struct sockaddr_in *addr;
    
    printf("\n=== Set IP Address ===\n");
    printf("Interface: %s\n", ifname);
    printf("IP: %s\n", ip);
    
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
        fprintf(stderr, "Địa chỉ IP không hợp lệ: %s\n", ip);
        close(sockfd);
        return -1;
    }
    
    if (ioctl(sockfd, SIOCSIFADDR, &ifr) < 0) {
        perror("ioctl SIOCSIFADDR");
        close(sockfd);
        return -1;
    }
    
    close(sockfd);
    printf("Đã set IP %s cho interface %s\n", ip, ifname);
    return 0;
}

/*
 * Set netmask cho interface
 */
int set_netmask(const char *ifname, const char *netmask) {
    int sockfd;
    struct ifreq ifr;
    struct sockaddr_in *addr;
    
    printf("\n=== Set Netmask ===\n");
    printf("Interface: %s\n", ifname);
    printf("Netmask: %s\n", netmask);
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    
    addr = (struct sockaddr_in *)&ifr.ifr_netmask;
    addr->sin_family = AF_INET;
    
    if (inet_pton(AF_INET, netmask, &addr->sin_addr) <= 0) {
        fprintf(stderr, "Netmask không hợp lệ: %s\n", netmask);
        close(sockfd);
        return -1;
    }
    
    if (ioctl(sockfd, SIOCSIFNETMASK, &ifr) < 0) {
        perror("ioctl SIOCSIFNETMASK");
        close(sockfd);
        return -1;
    }
    
    close(sockfd);
    printf("Đã set netmask %s cho interface %s\n", netmask, ifname);
    return 0;
}

/*
 * Hiển thị thông tin chi tiết của interface
 */
void show_interface_info(const char *ifname) {
    char ip[INET_ADDRSTRLEN];
    char mac[18];
    char netmask[INET_ADDRSTRLEN];
    char broadcast[INET_ADDRSTRLEN];
    int mtu;
    int is_up;
    
    printf("\n=== Thông tin Interface: %s ===\n\n", ifname);
    
    /* Trạng thái */
    is_up = is_interface_up(ifname);
    if (is_up >= 0) {
        printf("Trạng thái: %s\n", is_up ? "UP" : "DOWN");
    }
    
    /* MAC Address */
    if (get_mac_address(ifname, mac, sizeof(mac)) == 0) {
        printf("MAC Address: %s\n", mac);
    }
    
    /* IP Address */
    if (get_ip_address(ifname, ip, sizeof(ip)) == 0) {
        printf("IP Address: %s\n", ip);
    }
    
    /* Netmask */
    if (get_netmask(ifname, netmask, sizeof(netmask)) == 0) {
        printf("Netmask: %s\n", netmask);
    }
    
    /* Broadcast */
    if (get_broadcast_address(ifname, broadcast, sizeof(broadcast)) == 0) {
        printf("Broadcast: %s\n", broadcast);
    }
    
    /* MTU */
    if (get_mtu(ifname, &mtu) == 0) {
        printf("MTU: %d\n", mtu);
    }
    
    printf("\n");
}

/*
 * Sử dụng system() để thao tác với interface (cách đơn giản)
 */
void demo_system_commands(const char *ifname) {
    char cmd[256];
    
    printf("\n=== Demo sử dụng system() commands ===\n\n");
    
    /* Hiển thị thông tin interface */
    printf("1. Hiển thị thông tin interface:\n");
    snprintf(cmd, sizeof(cmd), "ip addr show %s", ifname);
    printf("Command: %s\n", cmd);
    system(cmd);
    printf("\n");
    
    /* Hiển thị routing table */
    printf("2. Hiển thị routing table:\n");
    printf("Command: ip route show\n");
    system("ip route show");
    printf("\n");
    
    /* Hiển thị statistics */
    printf("3. Hiển thị statistics:\n");
    snprintf(cmd, sizeof(cmd), "ip -s link show %s", ifname);
    printf("Command: %s\n", cmd);
    system(cmd);
    printf("\n");
}

void print_menu(void) {
    printf("\n========================================\n");
    printf("Network Interface Management Tool\n");
    printf("========================================\n");
    printf("1. Liệt kê tất cả interfaces\n");
    printf("2. Hiển thị thông tin interface cụ thể\n");
    printf("3. Set interface UP\n");
    printf("4. Set interface DOWN\n");
    printf("5. Set IP address\n");
    printf("6. Set netmask\n");
    printf("7. Demo system() commands\n");
    printf("0. Thoát\n");
    printf("========================================\n");
    printf("Chọn: ");
}

int main(void) {
    char ifname[MAX_INTERFACE_NAME];
    char ip[INET_ADDRSTRLEN];
    char netmask[INET_ADDRSTRLEN];
    int choice;
    char input[256];
    
    /* Kiểm tra quyền root */
    if (geteuid() != 0) {
        fprintf(stderr, "Chương trình cần quyền root để chạy.\n");
        fprintf(stderr, "Vui lòng chạy với sudo.\n");
        fprintf(stderr, "\nChế độ chỉ đọc (read-only) sẽ được kích hoạt.\n");
        fprintf(stderr, "Các chức năng set/up/down sẽ không khả dụng.\n\n");
    }
    
    while (1) {
        print_menu();
        
        if (fgets(input, sizeof(input), stdin) == NULL)
            break;
        
        choice = atoi(input);
        
        switch (choice) {
            case 1:
                list_interfaces();
                break;
                
            case 2:
                printf("Nhập tên interface (vd: eth0, lo, wlan0): ");
                if (fgets(ifname, sizeof(ifname), stdin) != NULL) {
                    ifname[strcspn(ifname, "\n")] = 0;
                    show_interface_info(ifname);
                }
                break;
                
            case 3:
                if (geteuid() != 0) {
                    printf("Cần quyền root! Chạy với sudo.\n");
                    break;
                }
                printf("Nhập tên interface: ");
                if (fgets(ifname, sizeof(ifname), stdin) != NULL) {
                    ifname[strcspn(ifname, "\n")] = 0;
                    set_interface_up(ifname);
                }
                break;
                
            case 4:
                if (geteuid() != 0) {
                    printf("Cần quyền root! Chạy với sudo.\n");
                    break;
                }
                printf("Nhập tên interface: ");
                if (fgets(ifname, sizeof(ifname), stdin) != NULL) {
                    ifname[strcspn(ifname, "\n")] = 0;
                    set_interface_down(ifname);
                }
                break;
                
            case 5:
                if (geteuid() != 0) {
                    printf("Cần quyền root! Chạy với sudo.\n");
                    break;
                }
                printf("Nhập tên interface: ");
                if (fgets(ifname, sizeof(ifname), stdin) != NULL) {
                    ifname[strcspn(ifname, "\n")] = 0;
                    printf("Nhập IP address: ");
                    if (fgets(ip, sizeof(ip), stdin) != NULL) {
                        ip[strcspn(ip, "\n")] = 0;
                        set_ip_address(ifname, ip);
                    }
                }
                break;
                
            case 6:
                if (geteuid() != 0) {
                    printf("Cần quyền root! Chạy với sudo.\n");
                    break;
                }
                printf("Nhập tên interface: ");
                if (fgets(ifname, sizeof(ifname), stdin) != NULL) {
                    ifname[strcspn(ifname, "\n")] = 0;
                    printf("Nhập netmask: ");
                    if (fgets(netmask, sizeof(netmask), stdin) != NULL) {
                        netmask[strcspn(netmask, "\n")] = 0;
                        set_netmask(ifname, netmask);
                    }
                }
                break;
                
            case 7:
                printf("Nhập tên interface: ");
                if (fgets(ifname, sizeof(ifname), stdin) != NULL) {
                    ifname[strcspn(ifname, "\n")] = 0;
                    demo_system_commands(ifname);
                }
                break;
                
            case 0:
                printf("\nThoát chương trình. Tạm biệt!\n");
                return 0;
                
            default:
                printf("Lựa chọn không hợp lệ.\n");
        }
    }
    
    return 0;
}
