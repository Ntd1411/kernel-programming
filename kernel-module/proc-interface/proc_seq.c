/*
 * proc_seq.c - Sequential file operations
 * 
 * Sử dụng seq_file để hiển thị danh sách dữ liệu lớn
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/smp.h>
#include <linux/cpumask.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kernel Linux Learning");
MODULE_DESCRIPTION("Proc sequential file operations");
MODULE_VERSION("1.0");

#define PROC_NAME "cpu_info"

static struct proc_dir_entry *proc_entry;

/* Ham start - bat dau iteration */
static void *cpu_seq_start(struct seq_file *s, loff_t *pos) {
    if (*pos >= num_possible_cpus())
        return NULL;
    return pos;
}

/* Ham next - di chuyen toi phan tu tiep theo */
static void *cpu_seq_next(struct seq_file *s, void *v, loff_t *pos) {
    (*pos)++;
    if (*pos >= num_possible_cpus())
        return NULL;
    return pos;
}

/* Ham stop - ket thuc iteration */
static void cpu_seq_stop(struct seq_file *s, void *v) {
    /* Khong can cleanup gi */
}

/* Ham show - hien thi du lieu cho moi phan tu */
static int cpu_seq_show(struct seq_file *s, void *v) {
    loff_t *cpu = (loff_t *)v;
    int cpu_id = *cpu;
    
    if (cpu_id == 0) {
        seq_printf(s, "%-5s %-10s %-10s\n", "CPU", "Online", "Present");
        seq_printf(s, "%-5s %-10s %-10s\n", "---", "------", "-------");
    }
    
    seq_printf(s, "%-5d %-10s %-10s\n",
               cpu_id,
               cpu_online(cpu_id) ? "Yes" : "No",
               cpu_present(cpu_id) ? "Yes" : "No");
    
    return 0;
}

static const struct seq_operations cpu_seq_ops = {
    .start = cpu_seq_start,
    .next  = cpu_seq_next,
    .stop  = cpu_seq_stop,
    .show  = cpu_seq_show
};

/* Ham open proc entry */
static int cpu_proc_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "ProcSeq: Mo /proc/%s tren CPU %d\n", 
           PROC_NAME, smp_processor_id());
    return seq_open(file, &cpu_seq_ops);
}

static const struct proc_ops cpu_proc_ops = {
    .proc_open    = cpu_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = seq_release
};

/* Ham khoi tao module */
static int __init proc_seq_init(void) {
    proc_entry = proc_create(PROC_NAME, 0444, NULL, &cpu_proc_ops);
    
    if (!proc_entry) {
        printk(KERN_ALERT "ProcSeq: Khong the tao /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }
    
    printk(KERN_INFO "ProcSeq: Tao /proc/%s thanh cong\n", PROC_NAME);
    printk(KERN_INFO "ProcSeq: Su dung 'cat /proc/%s' de xem thong tin CPU\n", PROC_NAME);
    return 0;
}

/* Ham cleanup module */
static void __exit proc_seq_exit(void) {
    proc_remove(proc_entry);
    printk(KERN_INFO "ProcSeq: Xoa /proc/%s thanh cong\n", PROC_NAME);
}

module_init(proc_seq_init);
module_exit(proc_seq_exit);
