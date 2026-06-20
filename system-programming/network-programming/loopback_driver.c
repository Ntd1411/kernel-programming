/*
 * loopback_driver.c - Loopback Network Driver Demo
 * 
 * Bài tập slide 13.1: Viết loopback network driver
 * - Cho phép up/down interface
 * - Gửi/nhận bản tin qua socket programming
 * - Demo sk_buff trong kernel
 * 
 * Biên dịch: make
 * Load: sudo insmod loopback_driver.ko
 * Unload: sudo rmmod loopback_driver
 * 
 * Test:
 *   sudo ip link set myloop0 up
 *   sudo ip addr add 10.0.0.1/24 dev myloop0
 *   ping 10.0.0.2
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/tcp.h>

#define DRIVER_NAME "myloop"
#define DEVICE_NAME "myloop0"

/* Private data cho network device */
struct loopback_priv {
    struct net_device_stats stats;
    struct sk_buff *skb;
    spinlock_t lock;
};

/*
 * Hàm mở network interface
 * Được gọi khi: ifconfig myloop0 up
 */
static int loopback_open(struct net_device *dev)
{
    printk(KERN_INFO "%s: Interface được mở (UP)\n", dev->name);
    
    /* Bắt đầu transmission queue */
    netif_start_queue(dev);
    
    return 0;
}

/*
 * Hàm đóng network interface
 * Được gọi khi: ifconfig myloop0 down
 */
static int loopback_stop(struct net_device *dev)
{
    printk(KERN_INFO "%s: Interface được đóng (DOWN)\n", dev->name);
    
    /* Dừng transmission queue */
    netif_stop_queue(dev);
    
    return 0;
}

/*
 * Hàm xử lý gửi packet
 * Được gọi khi user-space gửi data qua socket
 * 
 * Flow: sendto() -> kernel -> dev_queue_xmit() -> loopback_start_xmit()
 */
static netdev_tx_t loopback_start_xmit(struct sk_buff *skb, 
                                        struct net_device *dev)
{
    struct loopback_priv *priv = netdev_priv(dev);
    struct iphdr *iph;
    struct ethhdr *eth;
    
    printk(KERN_INFO "%s: Gửi packet, độ dài = %d bytes\n", 
           dev->name, skb->len);
    
    /* Phân tích Ethernet header */
    eth = eth_hdr(skb);
    if (eth) {
        printk(KERN_INFO "  Ethernet: proto=0x%04x\n", ntohs(eth->h_proto));
    }
    
    /* Phân tích IP header nếu có */
    if (skb->protocol == htons(ETH_P_IP)) {
        iph = ip_hdr(skb);
        if (iph) {
            printk(KERN_INFO "  IP: src=%pI4 dst=%pI4 proto=%d len=%d\n",
                   &iph->saddr, &iph->daddr, iph->protocol, ntohs(iph->tot_len));
        }
    }
    
    /* Cập nhật thống kê */
    spin_lock(&priv->lock);
    priv->stats.tx_packets++;
    priv->stats.tx_bytes += skb->len;
    spin_unlock(&priv->lock);
    
    /* 
     * Loopback: Gửi packet ngược lại chính nó
     * Giả lập việc nhận packet từ network
     */
    skb->dev = dev;
    skb->protocol = eth_type_trans(skb, dev);
    skb->ip_summed = CHECKSUM_UNNECESSARY;
    
    /* Đưa packet vào receive queue */
    netif_rx(skb);
    
    printk(KERN_INFO "%s: Packet đã được loop back\n", dev->name);
    
    return NETDEV_TX_OK;
}

/*
 * Lấy thống kê của device
 */
static struct net_device_stats *loopback_get_stats(struct net_device *dev)
{
    struct loopback_priv *priv = netdev_priv(dev);
    return &priv->stats;
}

/*
 * Cấu trúc operations cho network device
 */
static const struct net_device_ops loopback_netdev_ops = {
    .ndo_open            = loopback_open,
    .ndo_stop            = loopback_stop,
    .ndo_start_xmit      = loopback_start_xmit,
    .ndo_get_stats       = loopback_get_stats,
};

/*
 * Khởi tạo network device
 */
static void loopback_setup(struct net_device *dev)
{
    struct loopback_priv *priv;
    
    printk(KERN_INFO "Khởi tạo loopback device: %s\n", dev->name);
    
    /* Setup ethernet device */
    ether_setup(dev);
    
    /* Thiết lập operations */
    dev->netdev_ops = &loopback_netdev_ops;
    
    /* Thiết lập các thuộc tính */
    dev->flags |= IFF_NOARP;          /* Không cần ARP */
    dev->features |= NETIF_F_HW_CSUM; /* Hardware checksum */
    dev->mtu = 1500;                  /* MTU mặc định */
    
    /* Khởi tạo private data */
    priv = netdev_priv(dev);
    memset(priv, 0, sizeof(struct loopback_priv));
    spin_lock_init(&priv->lock);
    
    /* Tạo random MAC address */
    eth_hw_addr_random(dev);
    
    printk(KERN_INFO "%s: MAC address: %pM\n", dev->name, dev->dev_addr);
}

static struct net_device *loopback_dev;

/*
 * Module init - Load driver
 */
static int __init loopback_init(void)
{
    int ret;
    
    printk(KERN_INFO "=== Loopback Network Driver ===\n");
    printk(KERN_INFO "Đang load module...\n");
    
    /* Cấp phát network device */
    loopback_dev = alloc_netdev(sizeof(struct loopback_priv),
                                 DEVICE_NAME,
                                 NET_NAME_UNKNOWN,
                                 loopback_setup);
    
    if (!loopback_dev) {
        printk(KERN_ERR "Lỗi: Không thể cấp phát network device\n");
        return -ENOMEM;
    }
    
    /* Đăng ký network device */
    ret = register_netdev(loopback_dev);
    if (ret) {
        printk(KERN_ERR "Lỗi: Không thể đăng ký network device (ret=%d)\n", ret);
        free_netdev(loopback_dev);
        return ret;
    }
    
    printk(KERN_INFO "Đã tạo network interface: %s\n", loopback_dev->name);
    printk(KERN_INFO "Sử dụng:\n");
    printk(KERN_INFO "  sudo ip link set %s up\n", loopback_dev->name);
    printk(KERN_INFO "  sudo ip addr add 10.0.0.1/24 dev %s\n", loopback_dev->name);
    printk(KERN_INFO "  ping 10.0.0.2\n");
    
    return 0;
}

/*
 * Module exit - Unload driver
 */
static void __exit loopback_exit(void)
{
    printk(KERN_INFO "Đang unload loopback driver...\n");
    
    if (loopback_dev) {
        unregister_netdev(loopback_dev);
        free_netdev(loopback_dev);
        printk(KERN_INFO "Đã xóa network interface: %s\n", DEVICE_NAME);
    }
    
    printk(KERN_INFO "Module đã được unload\n");
}

module_init(loopback_init);
module_exit(loopback_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Học viện Kỹ thuật Mật mã");
MODULE_DESCRIPTION("Loopback Network Driver Demo - Bài tập slide 13.1");
MODULE_VERSION("1.0");
