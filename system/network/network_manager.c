/*
 * network_manager.c - Network Management User-space Application
 * 
 * Chức năng:
 * - Liệt kê các network interfaces
 * - Hiển thị thông tin chi tiết interface (IP, MAC, stats)
 * - Bật/tắt interface
 * - Thêm/xóa địa chỉ IP
 * - Giám sát traffic real-time
 * - Ping test
 * - Hiển thị routing table
 * - DNS lookup
 * 
 * Biên dịch: gcc -o network_manager network_manager.c
 * Chạy: sudo ./network_manager
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <ifaddrs.h>
#include <netdb.h>

#define MAX_INTERFACES 32
#define BUFFER_SIZE 4096
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

/* Cấu trúc lưu thông tin interface */
struct interface_info {
    char name[IFNAMSIZ];
    unsigned char mac[6];
    char ip[INET_ADDRSTRLEN];
    char netmask[INET_ADDRSTRLEN];
    char broadcast[INET_ADDRSTRLEN];
    int mtu;
    int flags;
    unsigned long rx_packets;
    unsigned long tx_packets;
    unsigned long rx_bytes;
    unsigned long tx_bytes;
    unsigned long rx_errors;
    unsigned long tx_errors;
};

/* Hiển thị menu chính */
void display_menu(void)
{
    printf("\n");
    printf("%s╔════════════════════════════════════════════════════════╗%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s║        NETWORK MANAGER - Quản lý Mạng User Space      ║%s\n", COLOR_CYAN, COLOR_RESET);
    printf("%s╚════════════════════════════════════════════════════════╝%s\n", COLOR_CYAN, COLOR_RESET);
    printf("\n");
    printf("  %s1.%s Liệt kê tất cả interfaces\n", COLOR_BOLD, COLOR_RESET);
    printf("  %s2.%s Hiển thị chi tiết interface\n", COLOR_BOLD, COLOR_RESET);
    printf("  %s3.%s Bật interface (bring up)\n", COLOR_BOLD, COLOR_RESET);
    printf("  %s4.%s Tắt interface (bring down)\n", COLOR_BOLD, COLOR_RESET);
    printf("  %s5.%s Thêm địa chỉ IP\n", COLOR_BOLD, COLOR_RESET);
    printf("  %s6.%s Xóa địa chỉ IP\n", COLOR_BOLD, COLOR_RESET);
    printf("  %s7.%s Liệt kê tất cả IP của interface\n", COLOR_BOLD, COLOR_RESET);
    printf("  %s8.%s Giám sát traffic real-time\n", COLOR_BOLD, COLOR_RESET);
    printf("  %s9.%s Ping test\n", COLOR_BOLD, COLOR_RESET);
    printf("  %s10.%s Hiển thị routing table\n", COLOR_BOLD, COLOR_RESET);
    printf("  %s11.%s DNS lookup\n", COLOR_BOLD, COLOR_RESET);
    printf("  %s12.%s Hiển thị socket statistics\n", COLOR_BOLD, COLOR_RESET);
    printf("  %s0.%s Thoát\n", COLOR_BOLD, COLOR_RESET);
    printf("\n");
    printf("Chọn chức năng: ");
}

/* Lấy danh sách tất cả interfaces */
int get_all_interfaces(struct interface_info *interfaces, int max_count)
{
    struct ifaddrs *ifaddr, *ifa;
    int count = 0;
    int sock;
    struct ifreq ifr;
    
    /* Tạo socket để ioctl */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Lỗi tạo socket");
        return -1;
    }
    
    /* Lấy danh sách interfaces */
    if (getifaddrs(&ifaddr) == -1) {
        perror("Lỗi getifaddrs");
        close(sock);
        return -1;
    }
    
    /* Duyệt qua từng interface */
    for (ifa = ifaddr; ifa != NULL && count < max_count; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;
        
        /* Chỉ xử lý IPv4 */
        if (ifa->ifa_addr->sa_family != AF_INET)
            continue;
        
        /* Kiểm tra interface đã có trong danh sách chưa */
        int found = 0;
        for (int i = 0; i < count; i++) {
            if (strcmp(interfaces[i].name, ifa->ifa_name) == 0) {
                found = 1;
                break;
            }
        }
        if (found)
            continue;
        
        /* Lưu tên interface */
        strncpy(interfaces[count].name, ifa->ifa_name, IFNAMSIZ - 1);
        
        /* Lấy địa chỉ IP */
        struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
        inet_ntop(AF_INET, &addr->sin_addr, interfaces[count].ip, INET_ADDRSTRLEN);
        
        /* Lấy netmask */
        if (ifa->ifa_netmask) {
            struct sockaddr_in *netmask = (struct sockaddr_in *)ifa->ifa_netmask;
            inet_ntop(AF_INET, &netmask->sin_addr, interfaces[count].netmask, INET_ADDRSTRLEN);
        }
        
        /* Lấy broadcast */
        if (ifa->ifa_ifu.ifu_broadaddr) {
            struct sockaddr_in *broadcast = (struct sockaddr_in *)ifa->ifa_ifu.ifu_broadaddr;
            inet_ntop(AF_INET, &broadcast->sin_addr, interfaces[count].broadcast, INET_ADDRSTRLEN);
        }
        
        /* Lấy MAC address và MTU qua ioctl */
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, ifa->ifa_name, IFNAMSIZ - 1);
        
        /* Lấy MAC */
        if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
            memcpy(interfaces[count].mac, ifr.ifr_hwaddr.sa_data, 6);
        }
        
        /* Lấy MTU */
        if (ioctl(sock, SIOCGIFMTU, &ifr) == 0) {
            interfaces[count].mtu = ifr.ifr_mtu;
        }
        
        /* Lấy flags */
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
            interfaces[count].flags = ifr.ifr_flags;
        }
        
        count++;
    }
    
    freeifaddrs(ifaddr);
    close(sock);
    
    return count;
}

/* Đọc statistics từ /proc/net/dev */
int get_interface_stats(const char *ifname, struct interface_info *info)
{
    FILE *fp;
    char line[256];
    char name[IFNAMSIZ];
    int found = 0;
    
    fp = fopen("/proc/net/dev", "r");
    if (!fp) {
        perror("Lỗi mở /proc/net/dev");
        return -1;
    }
    
    /* Bỏ qua 2 dòng header */
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);
    
    /* Đọc từng dòng */
    while (fgets(line, sizeof(line), fp)) {
        sscanf(line, "%[^:]:%lu %*u %lu %*u %*u %*u %*u %*u %lu %*u %lu",
               name,
               &info->rx_bytes,
               &info->rx_errors,
               &info->tx_bytes,
               &info->tx_errors);
        
        /* Xóa khoảng trắng */
        char *trim = name;
        while (*trim == ' ' || *trim == '\t')
            trim++;
        
        if (strcmp(trim, ifname) == 0) {
            found = 1;
            break;
        }
    }
    
    fclose(fp);
    return found ? 0 : -1;
}

/* Liệt kê tất cả interfaces */
void list_all_interfaces(void)
{
    struct interface_info interfaces[MAX_INTERFACES];
    int count, i;
    
    printf("\n%s=== DANH SÁCH NETWORK INTERFACES ===%s\n\n", COLOR_BOLD, COLOR_RESET);
    
    count = get_all_interfaces(interfaces, MAX_INTERFACES);
    if (count < 0) {
        printf("%sLỗi lấy danh sách interfaces%s\n", COLOR_RED, COLOR_RESET);
        return;
    }
    
    if (count == 0) {
        printf("Không tìm thấy interface nào\n");
        return;
    }
    
    printf("%-15s %-17s %-15s %-8s %s\n",
           "INTERFACE", "MAC ADDRESS", "IP ADDRESS", "MTU", "STATUS");
    printf("%-15s %-17s %-15s %-8s %s\n",
           "----------", "-----------", "----------", "---", "------");
    
    for (i = 0; i < count; i++) {
        const char *status_color = (interfaces[i].flags & IFF_UP) ? COLOR_GREEN : COLOR_RED;
        const char *status = (interfaces[i].flags & IFF_UP) ? "UP" : "DOWN";
        
        printf("%-15s %02x:%02x:%02x:%02x:%02x:%02x %-15s %-8d %s%s%s\n",
               interfaces[i].name,
               interfaces[i].mac[0], interfaces[i].mac[1],
               interfaces[i].mac[2], interfaces[i].mac[3],
               interfaces[i].mac[4], interfaces[i].mac[5],
               interfaces[i].ip,
               interfaces[i].mtu,
               status_color, status, COLOR_RESET);
    }
    
    printf("\nTổng số: %d interfaces\n", count);
}

/* Hiển thị chi tiết interface */
void show_interface_details(void)
{
    char ifname[IFNAMSIZ];
    struct interface_info info;
    struct ifreq ifr;
    int sock;
    
    printf("\nNhập tên interface: ");
    scanf("%s", ifname);
    
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Lỗi tạo socket");
        return;
    }
    
    /* Kiểm tra interface có tồn tại không */
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
        printf("%sInterface '%s' không tồn tại%s\n", COLOR_RED, ifname, COLOR_RESET);
        close(sock);
        return;
    }
    
    /* Lấy thông tin cơ bản */
    memset(&info, 0, sizeof(info));
    strncpy(info.name, ifname, IFNAMSIZ - 1);
    info.flags = ifr.ifr_flags;
    
    /* Lấy IP address */
    if (ioctl(sock, SIOCGIFADDR, &ifr) == 0) {
        struct sockaddr_in *addr = (struct sockaddr_in *)&ifr.ifr_addr;
        inet_ntop(AF_INET, &addr->sin_addr, info.ip, INET_ADDRSTRLEN);
    }
    
    /* Lấy netmask */
    if (ioctl(sock, SIOCGIFNETMASK, &ifr) == 0) {
        struct sockaddr_in *netmask = (struct sockaddr_in *)&ifr.ifr_netmask;
        inet_ntop(AF_INET, &netmask->sin_addr, info.netmask, INET_ADDRSTRLEN);
    }
    
    /* Lấy broadcast */
    if (ioctl(sock, SIOCGIFBRDADDR, &ifr) == 0) {
        struct sockaddr_in *broadcast = (struct sockaddr_in *)&ifr.ifr_broadaddr;
        inet_ntop(AF_INET, &broadcast->sin_addr, info.broadcast, INET_ADDRSTRLEN);
    }
    
    /* Lấy MAC */
    if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
        memcpy(info.mac, ifr.ifr_hwaddr.sa_data, 6);
    }
    
    /* Lấy MTU */
    if (ioctl(sock, SIOCGIFMTU, &ifr) == 0) {
        info.mtu = ifr.ifr_mtu;
    }
    
    /* Lấy statistics */
    get_interface_stats(ifname, &info);
    
    close(sock);
    
    /* Hiển thị thông tin */
    printf("\n%s=== CHI TIẾT INTERFACE: %s ===%s\n\n", COLOR_BOLD, ifname, COLOR_RESET);
    
    printf("%sTrạng thái:%s       ", COLOR_CYAN, COLOR_RESET);
    if (info.flags & IFF_UP) {
        printf("%sUP%s", COLOR_GREEN, COLOR_RESET);
        if (info.flags & IFF_RUNNING)
            printf(" %sRUNNING%s", COLOR_GREEN, COLOR_RESET);
    } else {
        printf("%sDOWN%s", COLOR_RED, COLOR_RESET);
    }
    printf("\n");
    
    printf("%sMAC Address:%s     %02x:%02x:%02x:%02x:%02x:%02x\n",
           COLOR_CYAN, COLOR_RESET,
           info.mac[0], info.mac[1], info.mac[2],
           info.mac[3], info.mac[4], info.mac[5]);
    
    printf("%sIP Address:%s      %s\n", COLOR_CYAN, COLOR_RESET, 
           strlen(info.ip) > 0 ? info.ip : "Chưa cấu hình");
    printf("%sNetmask:%s         %s\n", COLOR_CYAN, COLOR_RESET,
           strlen(info.netmask) > 0 ? info.netmask : "N/A");
    printf("%sBroadcast:%s       %s\n", COLOR_CYAN, COLOR_RESET,
           strlen(info.broadcast) > 0 ? info.broadcast : "N/A");
    printf("%sMTU:%s             %d bytes\n", COLOR_CYAN, COLOR_RESET, info.mtu);
    
    printf("\n%s=== STATISTICS ===%s\n", COLOR_BOLD, COLOR_RESET);
    printf("%sRX Bytes:%s        %lu (%.2f MB)\n", COLOR_CYAN, COLOR_RESET,
           info.rx_bytes, info.rx_bytes / (1024.0 * 1024.0));
    printf("%sTX Bytes:%s        %lu (%.2f MB)\n", COLOR_CYAN, COLOR_RESET,
           info.tx_bytes, info.tx_bytes / (1024.0 * 1024.0));
    printf("%sRX Errors:%s       %lu\n", COLOR_CYAN, COLOR_RESET, info.rx_errors);
    printf("%sTX Errors:%s       %lu\n", COLOR_CYAN, COLOR_RESET, info.tx_errors);
    
    printf("\n%s=== FLAGS ===%s\n", COLOR_BOLD, COLOR_RESET);
    if (info.flags & IFF_LOOPBACK) printf("  - LOOPBACK\n");
    if (info.flags & IFF_BROADCAST) printf("  - BROADCAST\n");
    if (info.flags & IFF_POINTOPOINT) printf("  - POINT-TO-POINT\n");
    if (info.flags & IFF_MULTICAST) printf("  - MULTICAST\n");
    if (info.flags & IFF_NOARP) printf("  - NO ARP\n");
    if (info.flags & IFF_PROMISC) printf("  - PROMISCUOUS\n");
}

/* Bật interface */
void bring_interface_up(void)
{
    char ifname[IFNAMSIZ];
    struct ifreq ifr;
    int sock;
    
    printf("\nNhập tên interface: ");
    scanf("%s", ifname);
    
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Lỗi tạo socket");
        return;
    }
    
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    
    /* Lấy flags hiện tại */
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
        perror("Lỗi lấy flags");
        close(sock);
        return;
    }
    
    /* Kiểm tra đã UP chưa */
    if (ifr.ifr_flags & IFF_UP) {
        printf("%sInterface '%s' đã ở trạng thái UP%s\n", 
               COLOR_YELLOW, ifname, COLOR_RESET);
        close(sock);
        return;
    }
    
    /* Set flag UP */
    ifr.ifr_flags |= IFF_UP;
    
    if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0) {
        perror("Lỗi bật interface");
        printf("%sGợi ý: Chạy với quyền root (sudo)%s\n", COLOR_YELLOW, COLOR_RESET);
        close(sock);
        return;
    }
    
    printf("%sĐã bật interface '%s' thành công%s\n", 
           COLOR_GREEN, ifname, COLOR_RESET);
    
    close(sock);
}

/* Tắt interface */
void bring_interface_down(void)
{
    char ifname[IFNAMSIZ];
    struct ifreq ifr;
    int sock;
    
    printf("\nNhập tên interface: ");
    scanf("%s", ifname);
    
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Lỗi tạo socket");
        return;
    }
    
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    
    /* Lấy flags hiện tại */
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
        perror("Lỗi lấy flags");
        close(sock);
        return;
    }
    
    /* Kiểm tra đã DOWN chưa */
    if (!(ifr.ifr_flags & IFF_UP)) {
        printf("%sInterface '%s' đã ở trạng thái DOWN%s\n",
               COLOR_YELLOW, ifname, COLOR_RESET);
        close(sock);
        return;
    }
    
    /* Clear flag UP */
    ifr.ifr_flags &= ~IFF_UP;
    
    if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0) {
        perror("Lỗi tắt interface");
        printf("%sGợi ý: Chạy với quyền root (sudo)%s\n", COLOR_YELLOW, COLOR_RESET);
        close(sock);
        return;
    }
    
    printf("%sĐã tắt interface '%s' thành công%s\n",
           COLOR_GREEN, ifname, COLOR_RESET);
    
    close(sock);
}

/* Chuyển netmask thành prefix length */
int netmask_to_prefix(const char *netmask)
{
    struct in_addr addr;
    unsigned int mask;
    int prefix = 0;
    
    if (inet_pton(AF_INET, netmask, &addr) != 1) {
        return -1;
    }
    
    mask = ntohl(addr.s_addr);
    
    while (mask) {
        prefix += mask & 1;
        mask >>= 1;
    }
    
    return prefix;
}

/* Thêm địa chỉ IP */
void add_ip_address(void)
{
    char ifname[IFNAMSIZ];
    char ip[INET_ADDRSTRLEN];
    char netmask[INET_ADDRSTRLEN];
    char cmd[256];
    int prefix;
    
    printf("\nNhập tên interface: ");
    scanf("%s", ifname);
    
    printf("Nhập địa chỉ IP: ");
    scanf("%s", ip);
    
    printf("Nhập prefix length (8-32) hoặc netmask (255.255.255.0): ");
    scanf("%s", netmask);
    
    /* Kiểm tra netmask là số hay dạng dotted decimal */
    if (strchr(netmask, '.')) {
        /* Dạng 255.255.255.0 - chuyển sang prefix */
        prefix = netmask_to_prefix(netmask);
        if (prefix < 0) {
            printf("%sNetmask không hợp lệ%s\n", COLOR_RED, COLOR_RESET);
            return;
        }
    } else {
        /* Dạng số 24 */
        prefix = atoi(netmask);
        if (prefix < 0 || prefix > 32) {
            printf("%sPrefix length không hợp lệ (phải từ 0-32)%s\n", 
                   COLOR_RED, COLOR_RESET);
            return;
        }
    }
    
    /* Sử dụng lệnh ip với CIDR notation */
    snprintf(cmd, sizeof(cmd), "ip addr add %s/%d dev %s 2>&1",
             ip, prefix, ifname);
    
    printf("\nĐang thêm địa chỉ IP...\n");
    
    FILE *fp = popen(cmd, "r");
    if (fp) {
        char output[256];
        int has_error = 0;
        
        while (fgets(output, sizeof(output), fp)) {
            /* Kiểm tra nếu là lỗi thật sự, không phải warning */
            if (strstr(output, "RTNETLINK answers:") || 
                strstr(output, "Cannot assign") ||
                strstr(output, "Error")) {
                printf("%s%s%s", COLOR_RED, output, COLOR_RESET);
                has_error = 1;
            }
        }
        
        int status = pclose(fp);
        
        if (status == 0 && !has_error) {
            printf("%sĐã thêm IP %s/%d vào interface '%s' thành công%s\n",
                   COLOR_GREEN, ip, prefix, ifname, COLOR_RESET);
        } else {
            printf("%sLỗi thêm địa chỉ IP%s\n", COLOR_RED, COLOR_RESET);
            printf("%sGợi ý: Chạy với quyền root (sudo) hoặc IP đã tồn tại%s\n", 
                   COLOR_YELLOW, COLOR_RESET);
        }
    } else {
        printf("%sLỗi thực thi lệnh%s\n", COLOR_RED, COLOR_RESET);
    }
}

/* Xóa địa chỉ IP */
void remove_ip_address(void)
{
    char ifname[IFNAMSIZ];
    char ip_input[64];
    char cmd[256];
    char ip[INET_ADDRSTRLEN];
    int prefix = 32;  /* Mặc định /32 cho single host */
    
    printf("\nNhập tên interface: ");
    scanf("%s", ifname);
    
    printf("Nhập địa chỉ IP cần xóa (có thể kèm /prefix, ví dụ: 1.1.1.1/24): ");
    scanf("%s", ip_input);
    
    /* Tách IP và prefix nếu có */
    char *slash = strchr(ip_input, '/');
    if (slash) {
        /* Có prefix trong input */
        *slash = '\0';
        strncpy(ip, ip_input, INET_ADDRSTRLEN - 1);
        prefix = atoi(slash + 1);
        
        if (prefix < 0 || prefix > 32) {
            printf("%sPrefix length không hợp lệ (phải từ 0-32)%s\n", 
                   COLOR_RED, COLOR_RESET);
            return;
        }
    } else {
        /* Không có prefix - mặc định /32 */
        strncpy(ip, ip_input, INET_ADDRSTRLEN - 1);
    }
    
    /* Sử dụng lệnh ip với CIDR notation rõ ràng */
    snprintf(cmd, sizeof(cmd), "ip addr del %s/%d dev %s 2>&1", 
             ip, prefix, ifname);
    
    printf("\nĐang xóa địa chỉ IP %s/%d...\n", ip, prefix);
    
    FILE *fp = popen(cmd, "r");
    if (fp) {
        char output[256];
        int has_error = 0;
        
        while (fgets(output, sizeof(output), fp)) {
            /* Bỏ qua warning về wildcard deletion */
            if (strstr(output, "wildcard deletion") || 
                strstr(output, "fix your scripts")) {
                continue;
            }
            
            /* Chỉ hiển thị lỗi thật sự */
            if (strstr(output, "RTNETLINK answers:") || 
                strstr(output, "Cannot") ||
                strstr(output, "Error")) {
                printf("%s%s%s", COLOR_RED, output, COLOR_RESET);
                has_error = 1;
            }
        }
        
        int status = pclose(fp);
        
        if (status == 0 && !has_error) {
            printf("%sĐã xóa IP %s/%d khỏi interface '%s' thành công%s\n",
                   COLOR_GREEN, ip, prefix, ifname, COLOR_RESET);
        } else {
            printf("%sLỗi xóa địa chỉ IP%s\n", COLOR_RED, COLOR_RESET);
            printf("%sGợi ý: Chạy với quyền root (sudo) hoặc IP không tồn tại%s\n", 
                   COLOR_YELLOW, COLOR_RESET);
        }
    } else {
        printf("%sLỗi thực thi lệnh%s\n", COLOR_RED, COLOR_RESET);
    }
}

/* Liệt kê tất cả địa chỉ IP của interface */
void list_interface_ips(void)
{
    char ifname[IFNAMSIZ];
    char cmd[256];
    FILE *fp;
    char line[512];
    int ip_count = 0;
    
    printf("\nNhập tên interface: ");
    scanf("%s", ifname);
    
    printf("\n%s=== TẤT CẢ ĐỊA CHỈ IP CỦA INTERFACE: %s ===%s\n\n", 
           COLOR_BOLD, ifname, COLOR_RESET);
    
    /* Sử dụng lệnh ip addr show để lấy tất cả IP */
    snprintf(cmd, sizeof(cmd), "ip addr show %s 2>&1", ifname);
    
    fp = popen(cmd, "r");
    if (!fp) {
        printf("%sLỗi thực thi lệnh%s\n", COLOR_RED, COLOR_RESET);
        return;
    }
    
    printf("%-5s %-20s %-20s %-10s\n", "STT", "IP ADDRESS", "NETMASK/PREFIX", "TYPE");
    printf("%-5s %-20s %-20s %-10s\n", "---", "----------", "--------------", "----");
    
    while (fgets(line, sizeof(line), fp)) {
        /* Tìm dòng chứa inet hoặc inet6 */
        if (strstr(line, "inet ") || strstr(line, "inet6 ")) {
            char ip[64], prefix[16], scope[32];
            char *type;
            
            /* Parse IPv4 */
            if (sscanf(line, " inet %s", ip) == 1) {
                type = "IPv4";
                ip_count++;
                
                /* Tách IP và prefix */
                char *slash = strchr(ip, '/');
                if (slash) {
                    *slash = '\0';
                    strncpy(prefix, slash + 1, sizeof(prefix) - 1);
                } else {
                    strncpy(prefix, "32", sizeof(prefix) - 1);
                }
                
                /* Lấy scope nếu có */
                if (strstr(line, "scope global")) {
                    strncpy(scope, "global", sizeof(scope) - 1);
                } else if (strstr(line, "scope host")) {
                    strncpy(scope, "host", sizeof(scope) - 1);
                } else if (strstr(line, "scope link")) {
                    strncpy(scope, "link", sizeof(scope) - 1);
                } else {
                    strncpy(scope, "unknown", sizeof(scope) - 1);
                }
                
                printf("%s%-5d%s %-20s %-20s %-10s\n", 
                       COLOR_GREEN, ip_count, COLOR_RESET,
                       ip, prefix, scope);
            }
            /* Parse IPv6 */
            else if (sscanf(line, " inet6 %s", ip) == 1) {
                type = "IPv6";
                ip_count++;
                
                /* Tách IP và prefix */
                char *slash = strchr(ip, '/');
                if (slash) {
                    *slash = '\0';
                    strncpy(prefix, slash + 1, sizeof(prefix) - 1);
                } else {
                    strncpy(prefix, "128", sizeof(prefix) - 1);
                }
                
                /* Lấy scope nếu có */
                if (strstr(line, "scope global")) {
                    strncpy(scope, "global", sizeof(scope) - 1);
                } else if (strstr(line, "scope host")) {
                    strncpy(scope, "host", sizeof(scope) - 1);
                } else if (strstr(line, "scope link")) {
                    strncpy(scope, "link", sizeof(scope) - 1);
                } else {
                    strncpy(scope, "unknown", sizeof(scope) - 1);
                }
                
                printf("%s%-5d%s %-20s %-20s %-10s\n", 
                       COLOR_CYAN, ip_count, COLOR_RESET,
                       ip, prefix, scope);
            }
        }
    }
    
    pclose(fp);
    
    if (ip_count == 0) {
        printf("\n%sInterface '%s' không có địa chỉ IP nào hoặc không tồn tại%s\n",
               COLOR_YELLOW, ifname, COLOR_RESET);
    } else {
        printf("\n%sTổng số: %d địa chỉ IP%s\n", COLOR_BOLD, ip_count, COLOR_RESET);
    }
}

/* Giám sát traffic real-time */
void monitor_traffic(void)
{
    char ifname[IFNAMSIZ];
    struct interface_info info1, info2;
    unsigned long rx_diff, tx_diff;
    double rx_rate, tx_rate;
    int count = 0;
    
    printf("\nNhập tên interface: ");
    scanf("%s", ifname);
    
    printf("\n%s=== GIÁM SÁT TRAFFIC REAL-TIME ===%s\n", COLOR_BOLD, COLOR_RESET);
    printf("Interface: %s\n", ifname);
    printf("Nhấn Ctrl+C để dừng...\n\n");
    
    /* Lấy stats ban đầu */
    memset(&info1, 0, sizeof(info1));
    if (get_interface_stats(ifname, &info1) < 0) {
        printf("%sKhông thể đọc statistics cho interface '%s'%s\n",
               COLOR_RED, ifname, COLOR_RESET);
        return;
    }
    
    printf("%-10s %15s %15s %15s %15s\n",
           "TIME", "RX BYTES", "TX BYTES", "RX RATE", "TX RATE");
    printf("%-10s %15s %15s %15s %15s\n",
           "----", "--------", "--------", "-------", "-------");
    
    while (1) {
        sleep(1);
        count++;
        
        /* Lấy stats mới */
        memset(&info2, 0, sizeof(info2));
        if (get_interface_stats(ifname, &info2) < 0) {
            printf("%sLỗi đọc statistics%s\n", COLOR_RED, COLOR_RESET);
            break;
        }
        
        /* Tính toán difference */
        rx_diff = info2.rx_bytes - info1.rx_bytes;
        tx_diff = info2.tx_bytes - info1.tx_bytes;
        
        /* Tính rate (bytes/sec) */
        rx_rate = rx_diff;
        tx_rate = tx_diff;
        
        /* Hiển thị */
        printf("%4ds      %10lu B    %10lu B    %10.2f KB/s %10.2f KB/s\n",
               count,
               info2.rx_bytes,
               info2.tx_bytes,
               rx_rate / 1024.0,
               tx_rate / 1024.0);
        
        /* Cập nhật stats cũ */
        info1 = info2;
        
        /* Giới hạn 60 giây */
        if (count >= 60) {
            printf("\nĐã giám sát 60 giây\n");
            break;
        }
    }
}

/* Ping test */
void ping_test(void)
{
    char host[256];
    char cmd[512];
    int count;
    
    printf("\nNhập địa chỉ IP hoặc hostname: ");
    scanf("%s", host);
    
    printf("Số lượng ping (1-100): ");
    scanf("%d", &count);
    
    if (count < 1 || count > 100) {
        printf("%sSố lượng không hợp lệ%s\n", COLOR_RED, COLOR_RESET);
        return;
    }
    
    printf("\n%s=== PING TEST ===%s\n", COLOR_BOLD, COLOR_RESET);
    printf("Đang ping %s (%d packets)...\n\n", host, count);
    
    snprintf(cmd, sizeof(cmd), "ping -c %d %s", count, host);
    system(cmd);
}

/* Hiển thị routing table */
void show_routing_table(void)
{
    FILE *fp;
    char line[512];
    
    printf("\n%s=== ROUTING TABLE ===%s\n\n", COLOR_BOLD, COLOR_RESET);
    
    fp = popen("ip route show", "r");
    if (!fp) {
        printf("%sLỗi đọc routing table%s\n", COLOR_RED, COLOR_RESET);
        return;
    }
    
    while (fgets(line, sizeof(line), fp)) {
        printf("%s", line);
    }
    
    pclose(fp);
    
    printf("\n%s=== IPv6 ROUTING TABLE ===%s\n\n", COLOR_BOLD, COLOR_RESET);
    
    fp = popen("ip -6 route show 2>/dev/null", "r");
    if (fp) {
        int has_route = 0;
        while (fgets(line, sizeof(line), fp)) {
            printf("%s", line);
            has_route = 1;
        }
        if (!has_route) {
            printf("Không có IPv6 route\n");
        }
        pclose(fp);
    }
}

/* DNS lookup */
void dns_lookup(void)
{
    char hostname[256];
    struct addrinfo hints, *result, *rp;
    int ret;
    char ipstr[INET6_ADDRSTRLEN];
    
    printf("\nNhập hostname: ");
    scanf("%s", hostname);
    
    printf("\n%s=== DNS LOOKUP ===%s\n", COLOR_BOLD, COLOR_RESET);
    printf("Hostname: %s\n\n", hostname);
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    ret = getaddrinfo(hostname, NULL, &hints, &result);
    if (ret != 0) {
        printf("%sLỗi DNS lookup: %s%s\n", 
               COLOR_RED, gai_strerror(ret), COLOR_RESET);
        return;
    }
    
    printf("%-10s %s\n", "TYPE", "IP ADDRESS");
    printf("%-10s %s\n", "----", "----------");
    
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        void *addr;
        const char *type;
        
        if (rp->ai_family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)rp->ai_addr;
            addr = &(ipv4->sin_addr);
            type = "IPv4";
        } else if (rp->ai_family == AF_INET6) {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)rp->ai_addr;
            addr = &(ipv6->sin6_addr);
            type = "IPv6";
        } else {
            continue;
        }
        
        inet_ntop(rp->ai_family, addr, ipstr, sizeof(ipstr));
        printf("%-10s %s\n", type, ipstr);
    }
    
    freeaddrinfo(result);
}

/* Hiển thị socket statistics */
void show_socket_stats(void)
{
    FILE *fp;
    char line[512];
    
    printf("\n%s=== SOCKET STATISTICS ===%s\n\n", COLOR_BOLD, COLOR_RESET);
    
    printf("%s--- TCP Sockets ---%s\n", COLOR_CYAN, COLOR_RESET);
    fp = popen("ss -t -a -n | head -20", "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            printf("%s", line);
        }
        pclose(fp);
    }
    
    printf("\n%s--- UDP Sockets ---%s\n", COLOR_CYAN, COLOR_RESET);
    fp = popen("ss -u -a -n | head -20", "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            printf("%s", line);
        }
        pclose(fp);
    }
    
    printf("\n%s--- Listening Sockets ---%s\n", COLOR_CYAN, COLOR_RESET);
    fp = popen("ss -l -n | head -20", "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            printf("%s", line);
        }
        pclose(fp);
    }
    
    printf("\n%s--- Socket Summary ---%s\n", COLOR_CYAN, COLOR_RESET);
    fp = popen("ss -s", "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            printf("%s", line);
        }
        pclose(fp);
    }
}

/* Hàm main */
int main(int argc, char *argv[])
{
    int choice;
    
    /* Kiểm tra quyền root cho một số chức năng */
    if (geteuid() != 0) {
        printf("%sCảnh báo: Một số chức năng yêu cầu quyền root%s\n",
               COLOR_YELLOW, COLOR_RESET);
        printf("Chạy với: sudo %s\n\n", argv[0]);
    }
    
    while (1) {
        display_menu();
        
        if (scanf("%d", &choice) != 1) {
            /* Clear input buffer */
            while (getchar() != '\n');
            printf("%sLựa chọn không hợp lệ%s\n", COLOR_RED, COLOR_RESET);
            continue;
        }
        
        switch (choice) {
            case 1:
                list_all_interfaces();
                break;
                
            case 2:
                show_interface_details();
                break;
                
            case 3:
                bring_interface_up();
                break;
                
            case 4:
                bring_interface_down();
                break;
                
            case 5:
                add_ip_address();
                break;
                
            case 6:
                remove_ip_address();
                break;
                
            case 7:
                list_interface_ips();
                break;
                
            case 8:
                monitor_traffic();
                break;
                
            case 9:
                ping_test();
                break;
                
            case 10:
                show_routing_table();
                break;
                
            case 11:
                dns_lookup();
                break;
                
            case 12:
                show_socket_stats();
                break;
                
            case 0:
                printf("\n%sTạm biệt!%s\n\n", COLOR_GREEN, COLOR_RESET);
                return 0;
                
            default:
                printf("%sLựa chọn không hợp lệ. Vui lòng chọn 0-12%s\n",
                       COLOR_RED, COLOR_RESET);
        }
        
        printf("\nNhấn Enter để tiếp tục...");
        while (getchar() != '\n');
        getchar();
    }
    
    return 0;
}
