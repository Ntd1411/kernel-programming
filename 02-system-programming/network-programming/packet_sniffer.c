/*
 * packet_sniffer.c - Bắt và phân tích gói tin mạng
 * 
 * Sử dụng raw socket để bắt các gói tin đi qua network interface
 * Yêu cầu quyền root
 * 
 * Biên dịch: gcc -o packet_sniffer packet_sniffer.c
 * Chạy: sudo ./packet_sniffer <interface>
 * Ví dụ: sudo ./packet_sniffer eth0
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
#include <netinet/ip_icmp.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>

#define BUFFER_SIZE 65536

void print_ethernet_header(unsigned char *buffer) {
    struct ethhdr *eth = (struct ethhdr *)buffer;
    
    printf("\n=== Ethernet Header ===\n");
    printf("Destination MAC: %.2X:%.2X:%.2X:%.2X:%.2X:%.2X\n",
           eth->h_dest[0], eth->h_dest[1], eth->h_dest[2],
           eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
    printf("Source MAC:      %.2X:%.2X:%.2X:%.2X:%.2X:%.2X\n",
           eth->h_source[0], eth->h_source[1], eth->h_source[2],
           eth->h_source[3], eth->h_source[4], eth->h_source[5]);
    printf("Protocol:        0x%04X", ntohs(eth->h_proto));
    
    switch (ntohs(eth->h_proto)) {
        case ETH_P_IP:
            printf(" (IPv4)\n");
            break;
        case ETH_P_IPV6:
            printf(" (IPv6)\n");
            break;
        case ETH_P_ARP:
            printf(" (ARP)\n");
            break;
        default:
            printf(" (Unknown)\n");
    }
}

void print_ip_header(unsigned char *buffer) {
    struct iphdr *iph = (struct iphdr *)(buffer + sizeof(struct ethhdr));
    struct in_addr src, dest;
    
    src.s_addr = iph->saddr;
    dest.s_addr = iph->daddr;
    
    printf("\n=== IP Header ===\n");
    printf("Version:         %d\n", iph->version);
    printf("Header Length:   %d bytes\n", iph->ihl * 4);
    printf("Type of Service: %d\n", iph->tos);
    printf("Total Length:    %d bytes\n", ntohs(iph->tot_len));
    printf("Identification:  %d\n", ntohs(iph->id));
    printf("TTL:             %d\n", iph->ttl);
    printf("Protocol:        %d", iph->protocol);
    
    switch (iph->protocol) {
        case IPPROTO_TCP:
            printf(" (TCP)\n");
            break;
        case IPPROTO_UDP:
            printf(" (UDP)\n");
            break;
        case IPPROTO_ICMP:
            printf(" (ICMP)\n");
            break;
        default:
            printf(" (Unknown)\n");
    }
    
    printf("Source IP:       %s\n", inet_ntoa(src));
    printf("Destination IP:  %s\n", inet_ntoa(dest));
}

void print_tcp_header(unsigned char *buffer) {
    struct iphdr *iph = (struct iphdr *)(buffer + sizeof(struct ethhdr));
    unsigned short iphdr_len = iph->ihl * 4;
    struct tcphdr *tcph = (struct tcphdr *)(buffer + sizeof(struct ethhdr) + iphdr_len);
    
    printf("\n=== TCP Header ===\n");
    printf("Source Port:     %u\n", ntohs(tcph->source));
    printf("Dest Port:       %u\n", ntohs(tcph->dest));
    printf("Sequence Number: %u\n", ntohl(tcph->seq));
    printf("Ack Number:      %u\n", ntohl(tcph->ack_seq));
    printf("Header Length:   %d bytes\n", tcph->doff * 4);
    printf("Flags:           ");
    if (tcph->urg) printf("URG ");
    if (tcph->ack) printf("ACK ");
    if (tcph->psh) printf("PSH ");
    if (tcph->rst) printf("RST ");
    if (tcph->syn) printf("SYN ");
    if (tcph->fin) printf("FIN ");
    printf("\n");
    printf("Window Size:     %u\n", ntohs(tcph->window));
    printf("Checksum:        0x%04X\n", ntohs(tcph->check));
}

void print_udp_header(unsigned char *buffer) {
    struct iphdr *iph = (struct iphdr *)(buffer + sizeof(struct ethhdr));
    unsigned short iphdr_len = iph->ihl * 4;
    struct udphdr *udph = (struct udphdr *)(buffer + sizeof(struct ethhdr) + iphdr_len);
    
    printf("\n=== UDP Header ===\n");
    printf("Source Port:     %u\n", ntohs(udph->source));
    printf("Dest Port:       %u\n", ntohs(udph->dest));
    printf("Length:          %u bytes\n", ntohs(udph->len));
    printf("Checksum:        0x%04X\n", ntohs(udph->check));
}

void print_icmp_header(unsigned char *buffer) {
    struct iphdr *iph = (struct iphdr *)(buffer + sizeof(struct ethhdr));
    unsigned short iphdr_len = iph->ihl * 4;
    struct icmphdr *icmph = (struct icmphdr *)(buffer + sizeof(struct ethhdr) + iphdr_len);
    
    printf("\n=== ICMP Header ===\n");
    printf("Type:            %d", icmph->type);
    
    switch (icmph->type) {
        case ICMP_ECHOREPLY:
            printf(" (Echo Reply)\n");
            break;
        case ICMP_ECHO:
            printf(" (Echo Request)\n");
            break;
        case ICMP_DEST_UNREACH:
            printf(" (Destination Unreachable)\n");
            break;
        case ICMP_TIME_EXCEEDED:
            printf(" (Time Exceeded)\n");
            break;
        default:
            printf(" (Unknown)\n");
    }
    
    printf("Code:            %d\n", icmph->code);
    printf("Checksum:        0x%04X\n", ntohs(icmph->checksum));
}

void process_packet(unsigned char *buffer, ssize_t size, int *packet_count) {
    struct ethhdr *eth = (struct ethhdr *)buffer;
    
    (*packet_count)++;
    
    printf("\n\n========================================\n");
    printf("Packet #%d (Size: %zd bytes)\n", *packet_count, size);
    printf("========================================");
    
    // Ethernet header
    print_ethernet_header(buffer);
    
    // Chỉ xử lý IPv4
    if (ntohs(eth->h_proto) == ETH_P_IP) {
        struct iphdr *iph = (struct iphdr *)(buffer + sizeof(struct ethhdr));
        
        // IP header
        print_ip_header(buffer);
        
        // Transport layer
        switch (iph->protocol) {
            case IPPROTO_TCP:
                print_tcp_header(buffer);
                break;
            case IPPROTO_UDP:
                print_udp_header(buffer);
                break;
            case IPPROTO_ICMP:
                print_icmp_header(buffer);
                break;
        }
    }
    
    printf("\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Sử dụng: %s <interface>\n", argv[0]);
        fprintf(stderr, "Ví dụ: %s eth0\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // Kiểm tra quyền root
    if (getuid() != 0) {
        fprintf(stderr, "Chương trình này cần quyền root\n");
        fprintf(stderr, "Chạy: sudo %s %s\n", argv[0], argv[1]);
        exit(EXIT_FAILURE);
    }
    
    char *interface = argv[1];
    int sockfd;
    unsigned char buffer[BUFFER_SIZE];
    struct sockaddr saddr;
    socklen_t saddr_len = sizeof(saddr);
    int packet_count = 0;
    
    // Tạo raw socket
    sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    printf("Raw socket được tạo thành công\n");
    
    // Bind socket vào interface cụ thể
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
    
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl SIOCGIFINDEX");
        fprintf(stderr, "Không tìm thấy interface: %s\n", interface);
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
    
    printf("Đang bắt gói tin trên interface: %s\n", interface);
    printf("Nhấn Ctrl+C để dừng\n");
    printf("========================================\n");
    
    // Bắt gói tin
    while (1) {
        ssize_t size = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, &saddr, &saddr_len);
        if (size < 0) {
            if (errno == EINTR) continue;
            perror("recvfrom");
            break;
        }
        
        process_packet(buffer, size, &packet_count);
    }
    
    printf("\n\nTổng số gói tin bắt được: %d\n", packet_count);
    
    close(sockfd);
    return 0;
}
