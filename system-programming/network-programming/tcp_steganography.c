/*
 * tcp_steganography.c - TCP/UDP Packet Steganography Module
 * 
 * Ẩn tin mật trong gói TCP/UDP bằng cách chỉnh sửa sk_buff
 * - Sử dụng Netfilter hook trước khi packet đến network driver
 * - Ẩn tin trong IP ID field hoặc TCP sequence number
 * - Mã hóa đơn giản để ẩn message
 * 
 * Biên dịch: make
 * Load: sudo insmod tcp_steganography.ko message="SECRET"
 * Unload: sudo rmmod tcp_steganography
 * 
 * Test: ping google.com (IP ID sẽ chứa hidden message)
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/skbuff.h>
#include <linux/string.h>

#define MAX_MESSAGE_LEN 256
#define MAGIC_MARKER 0xAB  /* Đánh dấu packet có hidden message */

static char hidden_message[MAX_MESSAGE_LEN] = "HIDDEN_DATA";
static int message_len = 11;
static int message_index = 0;
static struct nf_hook_ops nfho_out;

module_param_string(message, hidden_message, MAX_MESSAGE_LEN, 0644);
MODULE_PARM_DESC(message, "Message cần ẩn trong packets");

/*
 * Mã hóa 1 byte message thành 2 bytes để ẩn trong IP ID
 * Byte cao: magic marker + 4 bits cao của data
 * Byte thấp: 4 bits thấp của data + padding
 */
static void encode_byte_to_ip_id(unsigned char data_byte, unsigned short *ip_id)
{
    unsigned short encoded;
    
    /* Byte cao: MAGIC_MARKER (top 4 bits) + data (high 4 bits) */
    encoded = (MAGIC_MARKER & 0xF0) << 8;
    encoded |= ((data_byte & 0xF0) << 4);
    
    /* Byte thấp: data (low 4 bits) + random padding */
    encoded |= ((data_byte & 0x0F) << 4);
    encoded |= (jiffies & 0x0F);  /* Padding với timestamp bits */
    
    *ip_id = htons(encoded);
}

/*
 * Giải mã IP ID thành 1 byte message
 */
static int decode_ip_id_to_byte(unsigned short ip_id, unsigned char *data_byte)
{
    unsigned short decoded = ntohs(ip_id);
    unsigned char magic;
    
    /* Kiểm tra magic marker */
    magic = (decoded >> 12) & 0xF0;
    if (magic != (MAGIC_MARKER & 0xF0)) {
        return 0;  /* Không phải packet có hidden message */
    }
    
    /* Lấy 4 bits cao */
    *data_byte = (decoded >> 4) & 0xF0;
    
    /* Lấy 4 bits thấp */
    *data_byte |= (decoded >> 4) & 0x0F;
    
    return 1;
}

/*
 * Ẩn message vào TCP sequence number
 * Chỉ thay đổi 8 bits thấp nhất (ít ảnh hưởng đến TCP)
 */
static void hide_in_tcp_seq(struct tcphdr *tcph, unsigned char data_byte)
{
    u32 seq = ntohl(tcph->seq);
    
    /* Giữ nguyên 24 bits cao, thay đổi 8 bits thấp */
    seq = (seq & 0xFFFFFF00) | data_byte;
    
    tcph->seq = htonl(seq);
    
    /* Đánh dấu bằng cách set reserved bits */
    tcph->res1 = (MAGIC_MARKER >> 4) & 0x0F;
}

/*
 * Giải mã message từ TCP sequence number
 */
static int extract_from_tcp_seq(struct tcphdr *tcph, unsigned char *data_byte)
{
    u32 seq;
    unsigned char marker;
    
    /* Kiểm tra magic marker trong reserved bits */
    marker = tcph->res1 & 0x0F;
    if (marker != ((MAGIC_MARKER >> 4) & 0x0F)) {
        return 0;
    }
    
    seq = ntohl(tcph->seq);
    *data_byte = seq & 0xFF;
    
    return 1;
}

/*
 * Ẩn message vào UDP checksum
 * Lưu ý: Sẽ làm checksum sai, chỉ dùng cho demo
 */
static void hide_in_udp_checksum(struct udphdr *udph, unsigned char data_byte)
{
    u16 checksum = ntohs(udph->check);
    
    /* Lưu data vào 8 bits thấp của checksum */
    checksum = (checksum & 0xFF00) | data_byte;
    
    udph->check = htons(checksum);
}

/*
 * Tính lại IP checksum sau khi thay đổi IP ID
 */
static void recalculate_ip_checksum(struct iphdr *iph)
{
    unsigned short *ptr = (unsigned short *)iph;
    unsigned int sum = 0;
    int len = iph->ihl * 4;
    
    iph->check = 0;
    
    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }
    
    if (len == 1) {
        sum += *(unsigned char *)ptr;
    }
    
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    
    iph->check = ~sum;
}

/*
 * Netfilter hook function - Outgoing packets
 * Ẩn message vào packets trước khi gửi ra driver
 */
static unsigned int hook_outgoing(void *priv,
                                   struct sk_buff *skb,
                                   const struct nf_hook_state *state)
{
    struct iphdr *iph;
    struct tcphdr *tcph;
    struct udphdr *udph;
    unsigned char current_char;
    
    if (!skb) {
        return NF_ACCEPT;
    }
    
    /* Lấy IP header */
    iph = ip_hdr(skb);
    if (!iph) {
        return NF_ACCEPT;
    }
    
    /* Chỉ xử lý TCP và UDP */
    if (iph->protocol != IPPROTO_TCP && iph->protocol != IPPROTO_UDP) {
        return NF_ACCEPT;
    }
    
    /* Kiểm tra còn message để ẩn không */
    if (message_index >= message_len) {
        message_index = 0;  /* Reset để lặp lại message */
    }
    
    current_char = hidden_message[message_index];
    
    /* Phương pháp 1: Ẩn trong IP ID field (cho cả TCP và UDP) */
    encode_byte_to_ip_id(current_char, &iph->id);
    recalculate_ip_checksum(iph);
    
    /* Phương pháp 2: Ẩn trong TCP sequence number */
    if (iph->protocol == IPPROTO_TCP) {
        tcph = tcp_hdr(skb);
        if (tcph) {
            hide_in_tcp_seq(tcph, current_char);
            printk(KERN_DEBUG "Stego: Ẩn '%c' (0x%02x) vào TCP packet, index=%d\n",
                   (current_char >= 32 && current_char < 127) ? current_char : '.',
                   current_char, message_index);
        }
    }
    
    /* Phương pháp 3: Ẩn trong UDP checksum */
    if (iph->protocol == IPPROTO_UDP) {
        udph = udp_hdr(skb);
        if (udph && udph->check != 0) {
            hide_in_udp_checksum(udph, current_char);
            printk(KERN_DEBUG "Stego: Ẩn '%c' (0x%02x) vào UDP packet, index=%d\n",
                   (current_char >= 32 && current_char < 127) ? current_char : '.',
                   current_char, message_index);
        }
    }
    
    message_index++;
    
    return NF_ACCEPT;
}

/*
 * Module init
 */
static int __init stego_init(void)
{
    int ret;
    
    printk(KERN_INFO "=== TCP/UDP Steganography Module ===\n");
    printk(KERN_INFO "Đang load module...\n");
    
    message_len = strnlen(hidden_message, MAX_MESSAGE_LEN);
    printk(KERN_INFO "Hidden message: \"%s\" (%d bytes)\n", hidden_message, message_len);
    
    if (message_len == 0) {
        printk(KERN_WARNING "Message rỗng, sử dụng default message\n");
        strcpy(hidden_message, "HIDDEN_DATA");
        message_len = 11;
    }
    
    /* Đăng ký Netfilter hook cho outgoing packets */
    nfho_out.hook = hook_outgoing;
    nfho_out.hooknum = NF_INET_POST_ROUTING;  /* Trước khi đến driver */
    nfho_out.pf = PF_INET;
    nfho_out.priority = NF_IP_PRI_FIRST;
    
    ret = nf_register_net_hook(&init_net, &nfho_out);
    if (ret < 0) {
        printk(KERN_ERR "Lỗi: Không thể đăng ký Netfilter hook\n");
        return ret;
    }
    
    printk(KERN_INFO "Netfilter hook đã được đăng ký\n");
    printk(KERN_INFO "Module sẵn sàng. Đang ẩn message vào TCP/UDP packets...\n");
    printk(KERN_INFO "\nPhương pháp ẩn tin:\n");
    printk(KERN_INFO "  1. IP ID field (tất cả TCP/UDP)\n");
    printk(KERN_INFO "  2. TCP sequence number (chỉ TCP)\n");
    printk(KERN_INFO "  3. UDP checksum (chỉ UDP)\n");
    printk(KERN_INFO "\nSử dụng stego_reader để đọc message đã ẩn\n");
    
    return 0;
}

/*
 * Module exit
 */
static void __exit stego_exit(void)
{
    printk(KERN_INFO "Đang unload steganography module...\n");
    
    nf_unregister_net_hook(&init_net, &nfho_out);
    printk(KERN_INFO "Netfilter hook đã được hủy\n");
    
    printk(KERN_INFO "Tổng số packets đã xử lý: %d\n", message_index);
    printk(KERN_INFO "Module đã được unload\n");
}

module_init(stego_init);
module_exit(stego_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Học viện Kỹ thuật Mật mã");
MODULE_DESCRIPTION("TCP/UDP Steganography - Ẩn tin trong packets");
MODULE_VERSION("1.0");