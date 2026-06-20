/*
 * skbuff_demo.c - Demo sk_buff structure và manipulation
 * 
 * Minh họa slide 7-8: Struct skb và luồng di chuyển của gói tin
 * - Tạo và quản lý sk_buff
 * - Phân tích các trường trong sk_buff
 * - Demo add/remove header (push/pull)
 * - Demo clone và copy sk_buff
 * 
 * Biên dịch: make
 * Load: sudo insmod skbuff_demo.ko
 * Unload: sudo rmmod skbuff_demo
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>

/*
 * Demo 1: Tạo sk_buff và phân tích cấu trúc
 */
static void demo_skb_alloc(void)
{
    struct sk_buff *skb;
    
    printk(KERN_INFO "\n=== Demo 1: Tạo sk_buff ===\n");
    
    /* Cấp phát sk_buff với 1500 bytes data */
    skb = alloc_skb(1500, GFP_KERNEL);
    if (!skb) {
        printk(KERN_ERR "Lỗi: Không thể cấp phát sk_buff\n");
        return;
    }
    
    printk(KERN_INFO "sk_buff đã được tạo:\n");
    printk(KERN_INFO "  head: %p\n", skb->head);
    printk(KERN_INFO "  data: %p\n", skb->data);
    printk(KERN_INFO "  tail: %p\n", skb_tail_pointer(skb));
    printk(KERN_INFO "  end:  %p\n", skb_end_pointer(skb));
    printk(KERN_INFO "  len:  %d bytes\n", skb->len);
    printk(KERN_INFO "  truesize: %d bytes\n", skb->truesize);
    
    /* Giải phóng sk_buff */
    kfree_skb(skb);
    printk(KERN_INFO "sk_buff đã được giải phóng\n");
}

/*
 * Demo 2: Thêm data vào sk_buff
 */
static void demo_skb_put(void)
{
    struct sk_buff *skb;
    unsigned char *data;
    int i;
    
    printk(KERN_INFO "\n=== Demo 2: Thêm data vào sk_buff ===\n");
    
    skb = alloc_skb(1500, GFP_KERNEL);
    if (!skb) {
        return;
    }
    
    /* Reserve space cho headers */
    skb_reserve(skb, 64);
    printk(KERN_INFO "Reserved 64 bytes cho headers\n");
    printk(KERN_INFO "  data: %p, tail: %p\n", skb->data, skb_tail_pointer(skb));
    
    /* Thêm 100 bytes payload */
    data = skb_put(skb, 100);
    for (i = 0; i < 100; i++) {
        data[i] = i;
    }
    
    printk(KERN_INFO "Đã thêm 100 bytes payload\n");
    printk(KERN_INFO "  len: %d\n", skb->len);
    printk(KERN_INFO "  data: %p, tail: %p\n", skb->data, skb_tail_pointer(skb));
    
    kfree_skb(skb);
}

/*
 * Demo 3: Thêm headers (push) và remove headers (pull)
 */
static void demo_skb_push_pull(void)
{
    struct sk_buff *skb;
    struct ethhdr *eth;
    struct iphdr *iph;
    unsigned char *payload;
    
    printk(KERN_INFO "\n=== Demo 3: Push/Pull headers ===\n");
    
    skb = alloc_skb(1500, GFP_KERNEL);
    if (!skb) {
        return;
    }
    
    skb_reserve(skb, 128);
    
    /* Thêm payload trước */
    payload = skb_put(skb, 50);
    memset(payload, 0xAA, 50);
    printk(KERN_INFO "Payload: len=%d\n", skb->len);
    
    /* Push IP header */
    iph = (struct iphdr *)skb_push(skb, sizeof(struct iphdr));
    memset(iph, 0, sizeof(struct iphdr));
    iph->version = 4;
    iph->ihl = 5;
    iph->ttl = 64;
    iph->protocol = IPPROTO_TCP;
    
    printk(KERN_INFO "Đã push IP header: len=%d\n", skb->len);
    
    /* Push Ethernet header */
    eth = (struct ethhdr *)skb_push(skb, sizeof(struct ethhdr));
    memset(eth, 0, sizeof(struct ethhdr));
    eth->h_proto = htons(ETH_P_IP);
    
    printk(KERN_INFO "Đã push Ethernet header: len=%d\n", skb->len);
    printk(KERN_INFO "  Total: ETH(%zu) + IP(%zu) + Payload(50) = %d bytes\n",
           sizeof(struct ethhdr), sizeof(struct iphdr), skb->len);
    
    /* Pull Ethernet header */
    skb_pull(skb, sizeof(struct ethhdr));
    printk(KERN_INFO "Đã pull Ethernet header: len=%d\n", skb->len);
    
    /* Pull IP header */
    skb_pull(skb, sizeof(struct iphdr));
    printk(KERN_INFO "Đã pull IP header: len=%d (chỉ còn payload)\n", skb->len);
    
    kfree_skb(skb);
}

/*
 * Demo 4: Clone và copy sk_buff
 */
static void demo_skb_clone_copy(void)
{
    struct sk_buff *skb, *skb_clone, *skb_copy;
    unsigned char *data;
    
    printk(KERN_INFO "\n=== Demo 4: Clone và Copy sk_buff ===\n");
    
    /* Tạo sk_buff gốc */
    skb = alloc_skb(200, GFP_KERNEL);
    if (!skb) {
        return;
    }
    
    data = skb_put(skb, 100);
    memset(data, 0x11, 100);
    printk(KERN_INFO "SKB gốc: %p, len=%d\n", skb, skb->len);
    
    /* Clone: Chia sẻ data buffer */
    skb_clone = skb_clone(skb, GFP_KERNEL);
    if (skb_clone) {
        printk(KERN_INFO "SKB clone: %p, len=%d\n", skb_clone, skb_clone->len);
        printk(KERN_INFO "  Clone chia sẻ data buffer với gốc\n");
        printk(KERN_INFO "  skb->data: %p, clone->data: %p (same)\n", 
               skb->data, skb_clone->data);
        kfree_skb(skb_clone);
    }
    
    /* Copy: Tạo buffer riêng */
    skb_copy = skb_copy(skb, GFP_KERNEL);
    if (skb_copy) {
        printk(KERN_INFO "SKB copy: %p, len=%d\n", skb_copy, skb_copy->len);
        printk(KERN_INFO "  Copy có data buffer riêng\n");
        printk(KERN_INFO "  skb->data: %p, copy->data: %p (different)\n",
               skb->data, skb_copy->data);
        kfree_skb(skb_copy);
    }
    
    kfree_skb(skb);
}

/*
 * Demo 5: Phân tích packet với sk_buff
 */
static void demo_skb_packet_analysis(void)
{
    struct sk_buff *skb;
    struct ethhdr *eth;
    struct iphdr *iph;
    struct tcphdr *tcph;
    unsigned char *payload;
    
    printk(KERN_INFO "\n=== Demo 5: Phân tích packet ===\n");
    
    skb = alloc_skb(1500, GFP_KERNEL);
    if (!skb) {
        return;
    }
    
    skb_reserve(skb, 200);
    
    /* Tạo payload giả */
    payload = skb_put(skb, 20);
    memset(payload, 0xBB, 20);
    
    /* Thêm TCP header */
    tcph = (struct tcphdr *)skb_push(skb, sizeof(struct tcphdr));
    memset(tcph, 0, sizeof(struct tcphdr));
    tcph->source = htons(12345);
    tcph->dest = htons(80);
    tcph->doff = 5;
    
    /* Thêm IP header */
    iph = (struct iphdr *)skb_push(skb, sizeof(struct iphdr));
    memset(iph, 0, sizeof(struct iphdr));
    iph->version = 4;
    iph->ihl = 5;
    iph->protocol = IPPROTO_TCP;
    iph->saddr = 0x0100007f;  /* 127.0.0.1 */
    iph->daddr = 0x08080808;  /* 8.8.8.8 */
    iph->tot_len = htons(skb->len);
    
    /* Thêm Ethernet header */
    eth = (struct ethhdr *)skb_push(skb, sizeof(struct ethhdr));
    memset(eth, 0, sizeof(struct ethhdr));
    eth->h_proto = htons(ETH_P_IP);
    
    /* Set các pointers */
    skb->protocol = htons(ETH_P_IP);
    skb_reset_mac_header(skb);
    skb_set_network_header(skb, sizeof(struct ethhdr));
    skb_set_transport_header(skb, sizeof(struct ethhdr) + sizeof(struct iphdr));
    
    printk(KERN_INFO "Packet đã được tạo, tổng len=%d bytes\n", skb->len);
    printk(KERN_INFO "\nPhân tích packet:\n");
    
    /* Phân tích Ethernet */
    eth = eth_hdr(skb);
    printk(KERN_INFO "  Ethernet: proto=0x%04x\n", ntohs(eth->h_proto));
    
    /* Phân tích IP */
    iph = ip_hdr(skb);
    printk(KERN_INFO "  IP: version=%d, protocol=%d, len=%d\n",
           iph->version, iph->protocol, ntohs(iph->tot_len));
    printk(KERN_INFO "      src=%pI4, dst=%pI4\n", &iph->saddr, &iph->daddr);
    
    /* Phân tích TCP */
    tcph = tcp_hdr(skb);
    printk(KERN_INFO "  TCP: sport=%d, dport=%d\n",
           ntohs(tcph->source), ntohs(tcph->dest));
    
    /* Tính payload length */
    printk(KERN_INFO "  Payload: %d bytes\n",
           skb->len - sizeof(struct ethhdr) - sizeof(struct iphdr) - sizeof(struct tcphdr));
    
    kfree_skb(skb);
}

/*
 * Demo 6: Linear vs Non-linear sk_buff
 */
static void demo_skb_linear(void)
{
    struct sk_buff *skb;
    
    printk(KERN_INFO "\n=== Demo 6: Linear vs Non-linear ===\n");
    
    skb = alloc_skb(1500, GFP_KERNEL);
    if (!skb) {
        return;
    }
    
    skb_put(skb, 100);
    
    printk(KERN_INFO "sk_buff vừa tạo:\n");
    printk(KERN_INFO "  is_linear: %d\n", skb_is_linear(skb));
    printk(KERN_INFO "  data_len: %d (paged data)\n", skb->data_len);
    printk(KERN_INFO "  len: %d (total)\n", skb->len);
    printk(KERN_INFO "  nr_frags: %d\n", skb_shinfo(skb)->nr_frags);
    
    printk(KERN_INFO "\nLinear buffer: Tất cả data nằm liên tục trong memory\n");
    printk(KERN_INFO "Non-linear buffer: Data nằm rải rác (dùng cho large packets)\n");
    
    kfree_skb(skb);
}

/*
 * Module init
 */
static int __init skbuff_demo_init(void)
{
    printk(KERN_INFO "========================================\n");
    printk(KERN_INFO "sk_buff Demo Module\n");
    printk(KERN_INFO "Slide 7: struct skb - quản lý gói tin\n");
    printk(KERN_INFO "========================================\n");
    
    demo_skb_alloc();
    demo_skb_put();
    demo_skb_push_pull();
    demo_skb_clone_copy();
    demo_skb_packet_analysis();
    demo_skb_linear();
    
    printk(KERN_INFO "\n=== Demo hoàn thành ===\n");
    printk(KERN_INFO "Xem kết quả: dmesg | tail -100\n");
    
    return 0;
}

/*
 * Module exit
 */
static void __exit skbuff_demo_exit(void)
{
    printk(KERN_INFO "sk_buff demo module đã được unload\n");
}

module_init(skbuff_demo_init);
module_exit(skbuff_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Học viện Kỹ thuật Mật mã");
MODULE_DESCRIPTION("sk_buff Demo - Slide 7-8: Struct skb và luồng gói tin");
MODULE_VERSION("1.0");