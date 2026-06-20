/*
 * stego_reader.c - Steganography Reader (User-space)
 * 
 * Đọc hidden message từ TCP/UDP packets bằng raw socket
 * - Bắt packets với AF_PACKET
 * - Giải mã message từ IP ID, TCP seq, UDP checksum
 * - Hiển thị message đã ẩn
 * 
 * Biên dịch: gcc -o stego_reader stego_reader.c
 * Chạy: sudo ./stego_reader <interface>
 * Ví dụ: sudo ./stego_reader eth0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>

#define BUFFER_SIZE 65536
#define MAGIC_MARKER 0xAB
#define MAX_MESSAGE_LEN 1024

typedef struct {
    unsigned char data[MAX_MESSAGE_LEN];
    int length;
    int last_index;
} hidden_message_t;

static hidden_message_t ip_id_message = {0};
static hidden_message_t tcp_seq_message = {0};
static hidden_message_t udp_checksum_message = {0};

/*
 * Giải mã IP ID thành 1 byte
 */
static int decode_ip_id(unsigned short ip_id, unsigned char *data_byte)
{
    unsigned short decoded = ntohs(ip_id);
    unsigned char magic;
    
    /* Kiểm tra magic marker */
    magic = (decoded >> 12) & 0xF0;
    if (magic != (MAGIC_MARKER & 0xF0)) {
        return 0;
    }
    
    /* Lấy 4 bits cao */
    *data_byte = (decoded >> 4) & 0xF0;
    
    /* Lấy 4 bits thấp */
    *data_byte |= (decoded >> 4) & 0x0F;
    
    return 1;
}

/*
 * Giải mã TCP sequence number
 */
static int decode_tcp_seq(struct tcphdr *tcph, unsigned char *data_byte)
{
    unsigned char marker;
    u_int32_t seq;
    
    /* Kiểm tra magic marker trong reserved bits */
    marker = (tcph->th_x2 >> 4) & 0x0F;  /* res1 trong tcphdr */
    if (marker != ((MAGIC_MARKER >> 4) & 0x0F)) {
        return 0;
    }
    
    seq = ntohl(tcph->th_seq);
    *data_byte = seq & 0xFF;
    
    return 1;
}

/*
 * Giải mã UDP checksum
 */
static int decode_udp_checksum(struct udphdr *udph, unsigned char *data_byte)
{
    u_int16_t checksum = ntohs(udph->uh_sum);
    
    /* Lấy 8 bits thấp */
    *data_byte = checksum & 0xFF;
    
    return 1;
}

/*
 * Thêm byte vào message buffer
 */
static void add_byte_to_message(hidden_message_t *msg, unsigned char byte)
{
    if (msg->length < MAX_MESSAGE_LEN - 1) {
        msg->data[msg->length++] = byte;
        msg->data[msg->length] = '\0';
    }
}

/*
 * Hiển thị message đã thu thập
 */
static void print_message(const char *source, hidden_message_t *msg)
{
    int i;
    int printable_count = 0;
    
    if (msg->length == 0) {
        return;
    }
    
    /* Đếm ký tự printable */
    for (i = 0; i < msg->length; i++) {
        if (msg->data[i] >= 32 && msg->data[i] < 127) {
            printable_count++;
        }
    }
    
    /* Chỉ hiển thị nếu có ít nhất 50% ký tự printable */
    if (printable_count < msg->length / 2) {
        return;
    }
    
    printf("\n[%s] Hidden Message (%d bytes):\n", source, msg->length);
    printf("  ASCII: \"");
    for (i = 0; i < msg->length; i++) {
        if (msg->data[i] >= 32 && msg->data[i] < 127) {
            printf("%c", msg->data[i]);
        } else {
            printf(".");
        }
    }
    printf("\"\n");
    
    printf("  HEX: ");
    for (i = 0; i < msg->length && i < 64; i++) {
        printf("%02X ", msg->data[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n       ");
        }
    }
    if (msg->length > 64) {
        printf("... (truncated)");
    }
    printf("\n");
}

/*
 * Xử lý packet và trích xuất hidden data
 */
static void process_packet(unsigned char *buffer, int size)
{
    struct ethhdr *eth = (struct ethhdr *)buffer;
    struct iphdr *iph;
    struct tcphdr *tcph;
    struct udphdr *udph;
    unsigned char decoded_byte;
    static int packet_count = 0;
    
    packet_count++;
    
    /* Chỉ xử lý IPv4 */
    if (ntohs(eth->h_proto) != ETH_P_IP) {
        return;
    }
    
    iph = (struct iphdr *)(buffer + sizeof(struct ethhdr));
    
    /* Thử giải mã từ IP ID */
    if (decode_ip_id(iph->id, &decoded_byte)) {
        add_byte_to_message(&ip_id_message, decoded_byte);
        printf("[%d] IP ID: Tìm thấy byte 0x%02X '%c'\n", 
               packet_count, decoded_byte,
               (decoded_byte >= 32 && decoded_byte < 127) ? decoded_byte : '.');
        
        /* Hiển thị message mỗi 20 bytes */
        if (ip_id_message.length % 20 == 0) {
            print_message("IP ID", &ip_id_message);
        }
    }
    
    /* Xử lý TCP packets */
    if (iph->protocol == IPPROTO_TCP) {
        tcph = (struct tcphdr *)(buffer + sizeof(struct ethhdr) + (iph->ihl * 4));
        
        if (decode_tcp_seq(tcph, &decoded_byte)) {
            add_byte_to_message(&tcp_seq_message, decoded_byte);
            printf("[%d] TCP SEQ: Tìm thấy byte 0x%02X '%c'\n",
                   packet_count, decoded_byte,
                   (decoded_byte >= 32 && decoded_byte < 127) ? decoded_byte : '.');
            
            if (tcp_seq_message.length % 20 == 0) {
                print_message("TCP SEQ", &tcp_seq_message);
            }
        }
    }
    
    /* Xử lý UDP packets */
    if (iph->protocol == IPPROTO_UDP) {
        udph = (struct udphdr *)(buffer + sizeof(struct ethhdr) + (iph->ihl * 4));
        
        if (udph->uh_sum != 0 && decode_udp_checksum(udph, &decoded_byte)) {
            add_byte_to_message(&udp_checksum_message, decoded_byte);
            printf("[%d] UDP CHECKSUM: Tìm thấy byte 0x%02X '%c'\n",
                   packet_count, decoded_byte,
                   (decoded_byte >= 32 && decoded_byte < 127) ? decoded_byte : '.');
            
            if (udp_checksum_message.length % 20 == 0) {
                print_message("UDP CHECKSUM", &udp_checksum_message);
            }
        }
    }
}

/*
 * Hiển thị thống kê cuối cùng
 */
static void print_final_stats(void)
{
    printf("\n========================================\n");
    printf("Thống kê cuối cùng:\n");
    printf("========================================\n");
    
    if (ip_id_message.length > 0) {
        print_message("IP ID (FINAL)", &ip_id_message);
    } else {
        printf("\nIP ID: Không tìm thấy hidden message\n");
    }
    
    if (tcp_seq_message.length > 0) {
        print_message("TCP SEQ (FINAL)", &tcp_seq_message);
    } else {
        printf("\nTCP SEQ: Không tìm thấy hidden message\n");
    }
    
    if (udp_checksum_message.length > 0) {
        print_message("UDP CHECKSUM (FINAL)", &udp_checksum_message);
    } else {
        printf("\nUDP CHECKSUM: Không tìm thấy hidden message\n");
    }
    
    printf("\n========================================\n");
}

int main(int argc, char *argv[])
{
    int sockfd;
    unsigned char buffer[BUFFER_SIZE];
    struct sockaddr saddr;
    socklen_t saddr_len = sizeof(saddr);
    int data_size;
    int packet_count = 0;
    
    if (argc != 2) {
        fprintf(stderr, "Sử dụng: %s <interface>\n", argv[0]);
        fprintf(stderr, "Ví dụ: sudo %s eth0\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    const char *interface = argv[1];
    
    /* Kiểm tra quyền root */
    if (getuid() != 0) {
        fprintf(stderr, "Chương trình này cần quyền root\n");
        fprintf(stderr, "Chạy: sudo %s %s\n", argv[0], interface);
        exit(EXIT_FAILURE);
    }
    
    printf("========================================\n");
    printf("Steganography Reader\n");
    printf("========================================\n");
    printf("Interface: %s\n", interface);
    printf("Đang bắt packets và tìm hidden messages...\n");
    printf("Nhấn Ctrl+C để dừng\n\n");
    
    /* Tạo raw socket */
    sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    /* Bind socket vào interface cụ thể */
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
    
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl SIOCGIFINDEX");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifr.ifr_ifindex;
    sll.sll_protocol = htons(ETH_P_ALL);
    
    if (bind(sockfd, (struct sockaddr *)&sll, sizeof(sll)) < 0) {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    printf("Socket đã bind vào interface %s\n", interface);
    printf("Bắt đầu capture packets...\n\n");
    
    /* Bắt và xử lý packets */
    while (1) {
        data_size = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, 
                            &saddr, &saddr_len);
        
        if (data_size < 0) {
            if (errno == EINTR) {
                /* Bị ngắt bởi signal (Ctrl+C) */
                break;
            }
            perror("recvfrom");
            continue;
        }
        
        packet_count++;
        process_packet(buffer, data_size);
        
        /* Hiển thị tiến trình mỗi 100 packets */
        if (packet_count % 100 == 0) {
            printf("\r[Info] Đã xử lý %d packets...", packet_count);
            fflush(stdout);
        }
    }
    
    printf("\n\nKết thúc capture. Tổng packets: %d\n", packet_count);
    
    /* Hiển thị thống kê cuối */
    print_final_stats();
    
    close(sockfd);
    return 0;
}