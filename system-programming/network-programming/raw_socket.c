/*
 * raw_socket.c - Raw socket programming để tạo custom packet
 * 
 * Raw socket cho phép tạo và gửi packet ở tầng network layer
 * Yêu cầu quyền root để chạy
 * 
 * Biên dịch: gcc -o raw_socket raw_socket.c
 * Chạy: sudo ./raw_socket <destination_ip>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PACKET_SIZE 64

// Tính checksum cho IP header
unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;
    
    for (sum = 0; len > 1; len -= 2) {
        sum += *buf++;
    }
    
    if (len == 1) {
        sum += *(unsigned char *)buf;
    }
    
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    
    return result;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Sử dụng: %s <destination_ip>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // Kiểm tra quyền root
    if (getuid() != 0) {
        fprintf(stderr, "Chương trình này cần quyền root để chạy\n");
        fprintf(stderr, "Chạy: sudo %s %s\n", argv[0], argv[1]);
        exit(EXIT_FAILURE);
    }
    
    char *dest_ip = argv[1];
    int sockfd;
    char packet[PACKET_SIZE];
    struct sockaddr_in dest_addr;
    
    // Tạo raw socket
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    printf("Raw socket được tạo thành công (fd=%d)\n", sockfd);
    
    // Cho phép tự tạo IP header
    int one = 1;
    if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("setsockopt IP_HDRINCL");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    // Cấu trúc destination
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, dest_ip, &dest_addr.sin_addr) <= 0) {
        fprintf(stderr, "Địa chỉ IP không hợp lệ: %s\n", dest_ip);
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    // Chuẩn bị packet
    memset(packet, 0, PACKET_SIZE);
    
    // IP Header
    struct iphdr *iph = (struct iphdr *)packet;
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof(struct iphdr) + sizeof(struct icmphdr);
    iph->id = htons(54321);
    iph->frag_off = 0;
    iph->ttl = 64;
    iph->protocol = IPPROTO_ICMP;
    iph->check = 0; // Sẽ tính sau
    iph->saddr = inet_addr("192.168.1.1"); // Source IP
    iph->daddr = dest_addr.sin_addr.s_addr;
    
    // ICMP Header (Echo Request)
    struct icmphdr *icmph = (struct icmphdr *)(packet + sizeof(struct iphdr));
    icmph->type = ICMP_ECHO;
    icmph->code = 0;
    icmph->un.echo.id = htons(1234);
    icmph->un.echo.sequence = htons(1);
    icmph->checksum = 0;
    icmph->checksum = checksum(icmph, sizeof(struct icmphdr));
    
    // Tính IP checksum
    iph->check = checksum(packet, iph->tot_len);
    
    printf("\n--- Thông tin Packet ---\n");
    printf("Destination IP: %s\n", dest_ip);
    printf("Protocol: ICMP (Echo Request)\n");
    printf("Packet size: %d bytes\n", ntohs(iph->tot_len));
    printf("TTL: %d\n", iph->ttl);
    printf("------------------------\n\n");
    
    // Gửi packet
    printf("Đang gửi packet...\n");
    ssize_t sent = sendto(sockfd, packet, iph->tot_len, 0,
                          (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    
    if (sent < 0) {
        perror("sendto");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    printf("Đã gửi %zd bytes\n", sent);
    printf("Packet đã được gửi thành công!\n");
    
    // Gửi nhiều packet
    printf("\nGửi 5 ICMP Echo Request packets...\n");
    for (int i = 0; i < 5; i++) {
        icmph->un.echo.sequence = htons(i + 2);
        icmph->checksum = 0;
        icmph->checksum = checksum(icmph, sizeof(struct icmphdr));
        
        sent = sendto(sockfd, packet, iph->tot_len, 0,
                      (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        
        if (sent > 0) {
            printf("  Packet %d gửi thành công (seq=%d)\n", i + 1, i + 2);
        } else {
            perror("  sendto");
        }
        
        usleep(500000); // 0.5 giây
    }
    
    printf("\nHoàn thành!\n");
    
    close(sockfd);
    return 0;
}
