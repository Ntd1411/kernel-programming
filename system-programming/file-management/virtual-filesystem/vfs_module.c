/*
 * vfs_module.c - Virtual Filesystem Module đơn giản
 * 
 * Module tạo một filesystem ảo trong kernel space cho mục đích học tập.
 * Filesystem này chỉ tồn tại trong bộ nhớ, không lưu trữ dữ liệu vật lý.
 * 
 * Các kiến thức VFS được sử dụng:
 * 
 * 1. Superblock (struct super_block, super_operations):
 *    - Đại diện cho một filesystem instance đã mount
 *    - Quản lý metadata: magic number, block size, root dentry
 *    - Cung cấp các operations: statfs, drop_inode, evict_inode, put_super
 * 
 * 2. Inode (struct inode, inode_operations):
 *    - Đại diện cho một file/directory trong filesystem
 *    - Chứa metadata: số hiệu inode, quyền truy cập, timestamps, link count
 *    - Phân biệt giữa directory inode (có .lookup) và file inode (không có .lookup)
 *    - Sử dụng iget_locked() với số hiệu inode cố định để tránh race condition
 * 
 * 3. Dentry (struct dentry):
 *    - Đại diện cho một đường dẫn trong cây thư mục
 *    - Liên kết tên file với inode tương ứng
 *    - Được tạo bởi d_make_root() (root) và d_add() (file thường)
 * 
 * 4. File Operations (struct file_operations):
 *    - Định nghĩa các thao tác trên file: read, write, seek
 *    - Directory operations: iterate_shared để liệt kê nội dung thư mục
 * 
 * 5. Filesystem Registration:
 *    - Đăng ký filesystem type với kernel qua register_filesystem()
 *    - Cung cấp hàm mount để tạo superblock instance
 *    - Sử dụng mount_nodev() vì không cần block device
 * 
 * 6. Inode Lifecycle:
 *    - drop_inode: quyết định khi nào giải phóng inode
 *    - evict_inode: dọn dẹp dữ liệu khi xóa inode
 *    - set_nlink(): quản lý reference count của inode
 * 
 * Lưu ý kỹ thuật quan trọng:
 * - Phải dùng iget_locked() với số hiệu inode cố định ngay từ đầu
 * - KHÔNG được thay đổi i_ino sau khi inode đã được insert vào superblock
 * - File thường cần inode_operations riêng, không dùng simple_dir_inode_operations
 * 
 * Biên dịch: make
 * Load module: sudo insmod vfs_module.ko
 * Mount: sudo mount -t simplefs none /mnt/simplefs
 * Umount: sudo umount /mnt/simplefs
 * Unload: sudo rmmod vfs_module
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/backing-dev.h>
#include <linux/sched.h>
#include <linux/parser.h>
#include <linux/magic.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mnt_idmapping.h>
#include <linux/statfs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kernel Linux Learning");
MODULE_DESCRIPTION("Virtual filesystem module don gian");
MODULE_VERSION("1.1");

#define SIMPLEFS_MAGIC 0x19920342
#define SIMPLEFS_DEFAULT_MODE 0755

/* Nội dung các file ảo trong filesystem */
static const char hello_content[] = "Hello from SimplFS!\nThis is a virtual file in kernel space.\n";
static const char info_content[] = "SimplFS Information:\n"
                                   "- Type: Virtual Filesystem\n"
                                   "- Mode: Read-only\n"
                                   "- Storage: Memory only\n"
                                   "- Purpose: Learning and demonstration\n";

/* Số hiệu inode cố định */
enum {
    SIMPLEFS_ROOT_INO = 1,
    SIMPLEFS_HELLO_INO = 2,
    SIMPLEFS_INFO_INO = 3,
};

/* Khai báo trước các hàm và cấu trúc */
static struct inode *simplefs_get_inode(struct super_block *sb,
                                        const struct inode *dir,
                                        umode_t mode,
                                        unsigned long ino,
                                        dev_t dev);

static const struct inode_operations simplefs_dir_inode_operations;
static const struct inode_operations simplefs_file_inode_operations;
static const struct file_operations simplefs_file_operations;
static const struct super_operations simplefs_super_ops;

/* Hàm đọc dữ liệu từ file ảo */
static ssize_t simplefs_read(struct file *filp, char __user *buf,
                            size_t len, loff_t *ppos)
{
    struct inode *inode = file_inode(filp);
    const char *content = NULL;
    size_t content_len = 0;
    size_t to_copy;

    printk(KERN_INFO "SimpleFS: Doc file inode %lu, offset %lld, len %zu\n",
           inode->i_ino, *ppos, len);

    switch (inode->i_ino) {
        case SIMPLEFS_HELLO_INO:
            content = hello_content;
            content_len = sizeof(hello_content) - 1;
            break;
        case SIMPLEFS_INFO_INO:
            content = info_content;
            content_len = sizeof(info_content) - 1;
            break;
        default:
            return -EINVAL;
    }

    if (*ppos >= content_len)
        return 0;

    to_copy = min(len, content_len - (size_t)*ppos);

    if (copy_to_user(buf, content + *ppos, to_copy))
        return -EFAULT;

    *ppos += to_copy;

    printk(KERN_INFO "SimpleFS: Da doc %zu bytes tu inode %lu\n", to_copy, inode->i_ino);

    return to_copy;
}

/* File operations cho file thường */
static const struct file_operations simplefs_file_operations = {
    .read = simplefs_read,
    .llseek = generic_file_llseek,
};

/* Inode operations cho file thường - struct rỗng, file thường không cần .lookup */
static const struct inode_operations simplefs_file_inode_operations = {
};

/* Hàm lấy inode cho file thường */
static struct inode *simplefs_iget(struct super_block *sb, unsigned long ino)
{
    struct inode *inode;

    inode = iget_locked(sb, ino);
    if (!inode)
        return ERR_PTR(-ENOMEM);

    if (!(inode->i_state & I_NEW))
        return inode;

    inode_init_owner(&nop_mnt_idmap, inode, NULL, S_IFREG | 0444);
    simple_inode_init_ts(inode);

    /* Sử dụng inode operations riêng cho file thường, không phải directory ops */
    inode->i_op = &simplefs_file_inode_operations;
    inode->i_fop = &simplefs_file_operations;

    /* Đặt nlink rõ ràng, không dựa vào giá trị mặc định */
    set_nlink(inode, 1);

    unlock_new_inode(inode);

    printk(KERN_INFO "SimpleFS: Tao inode %lu (nlink=%u)\n", ino, inode->i_nlink);

    return inode;
}

/* Hàm lookup để tìm file trong thư mục */
static struct dentry *simplefs_lookup(struct inode *dir, struct dentry *dentry,
                                     unsigned int flags)
{
    struct inode *inode = NULL;

    printk(KERN_INFO "SimpleFS: Lookup '%s' trong thu muc\n", dentry->d_name.name);

    if (dir->i_ino != SIMPLEFS_ROOT_INO)
        return ERR_PTR(-ENOENT);

    if (strcmp(dentry->d_name.name, "hello") == 0) {
        inode = simplefs_iget(dir->i_sb, SIMPLEFS_HELLO_INO);
    } else if (strcmp(dentry->d_name.name, "info") == 0) {
        inode = simplefs_iget(dir->i_sb, SIMPLEFS_INFO_INO);
    }

    if (IS_ERR(inode))
        return ERR_CAST(inode);

    d_add(dentry, inode);
    return NULL;
}

/* Hàm iterate để liệt kê nội dung thư mục */
static int simplefs_iterate(struct file *file, struct dir_context *ctx)
{
    struct inode *inode = file_inode(file);

    printk(KERN_INFO "SimpleFS: Iterate thu muc inode %lu, pos %lld\n",
           inode->i_ino, ctx->pos);

    if (inode->i_ino != SIMPLEFS_ROOT_INO)
        return -ENOENT;

    if (!dir_emit_dots(file, ctx))
        return 0;

    if (ctx->pos == 2) {
        if (!dir_emit(ctx, "hello", 5, SIMPLEFS_HELLO_INO, DT_REG))
            return 0;
        ctx->pos++;
    }

    if (ctx->pos == 3) {
        if (!dir_emit(ctx, "info", 4, SIMPLEFS_INFO_INO, DT_REG))
            return 0;
        ctx->pos++;
    }

    return 0;
}

/* File operations cho thư mục */
static const struct file_operations simplefs_dir_operations = {
    .iterate_shared = simplefs_iterate,
    .llseek = default_llseek,
};

/* Inode operations cho thư mục */
static const struct inode_operations simplefs_dir_inode_operations = {
    .lookup = simplefs_lookup,
};

/*
 * Hàm tạo inode với số hiệu cố định
 * 
 * Lưu ý quan trọng về việc tránh treo umount:
 * 
 * Bản gốc dùng new_inode(sb) -> inode được insert vào sb->s_inodes list
 * NGAY LẬP TỨC với i_ino = get_next_ino() (ví dụ 9218). Sau đó caller
 * (simplefs_fill_super) gán lại root_inode->i_ino = 1.
 * 
 * Đổi i_ino SAU KHI inode đã nằm trong các cấu trúc quản lý của superblock
 * là không an toàn: bất kỳ tra cứu/iterate nào dựa trên i_ino (ví dụ trong
 * quá trình shrink_dcache_for_umount, generic_shutdown_super đi qua
 * sb->s_inodes để evict từng inode) có thể không nhất quán, gây vòng lặp
 * hoặc lookup sai khiến umount treo (quan sát thực tế: process umount ở
 * trạng thái R - đang chạy/spin, không phải D - bị block bởi lock).
 * 
 * Cách sửa: KHÔNG dùng get_next_ino() cho các inode có i_ino cố định
 * (root=1, hello=2, info=3). Truyền ino mong muốn vào thẳng trước khi insert,
 * dùng iget_locked() giống cách simplefs_iget() đã làm cho hello/info - tạo
 * một hàm chung dùng lại cho root.
 */
static struct inode *simplefs_get_inode(struct super_block *sb,
                                       const struct inode *dir,
                                       umode_t mode,
                                       unsigned long ino,
                                       dev_t dev)
{
    struct inode *inode;

    if (ino) {
        /* Inode có số hiệu cố định (root, hello, info): dùng iget_locked
         * để i_ino đúng NGAY TỪ ĐẦU, tránh đổi sau */
        inode = iget_locked(sb, ino);
        if (!inode)
            return NULL;
        if (!(inode->i_state & I_NEW))
            return inode;
    } else {
        inode = new_inode(sb);
        if (!inode)
            return NULL;
        inode->i_ino = get_next_ino();
    }

    inode_init_owner(&nop_mnt_idmap, inode, dir, mode);

    simple_inode_init_ts(inode);

    switch (mode & S_IFMT) {
        case S_IFDIR:
            inode->i_op = &simplefs_dir_inode_operations;
            inode->i_fop = &simplefs_dir_operations;
            set_nlink(inode, 2);
            break;
        case S_IFREG:
            /* Cũng dùng inode operations riêng cho file thường tại đây */
            inode->i_op = &simplefs_file_inode_operations;
            inode->i_fop = &simplefs_file_operations;
            set_nlink(inode, 1);
            break;
        default:
            init_special_inode(inode, mode, dev);
            break;
    }

    if (ino)
        unlock_new_inode(inode);

    printk(KERN_INFO "SimpleFS: Tao inode %lu voi mode 0%o (nlink=%u)\n",
           inode->i_ino, mode, inode->i_nlink);

    return inode;
}

/* Hàm lấy thông tin filesystem */
static int simplefs_statfs(struct dentry *dentry, struct kstatfs *buf)
{
    printk(KERN_INFO "SimpleFS: Goi statfs\n");

    buf->f_type = SIMPLEFS_MAGIC;
    buf->f_bsize = PAGE_SIZE;
    buf->f_namelen = 255;
    buf->f_blocks = 0;
    buf->f_bfree = 0;
    buf->f_bavail = 0;
    buf->f_files = 3;
    buf->f_ffree = 0;

    return 0;
}

/* Hàm xử lý khi drop inode */
static int simplefs_drop_inode(struct inode *inode)
{
    printk(KERN_INFO "SimpleFS: Drop inode %lu (nlink=%u, count=%u)\n",
           inode->i_ino, inode->i_nlink, inode->i_count.counter);
    return generic_drop_inode(inode);
}

/* Hàm xử lý khi evict inode */
static void simplefs_evict_inode(struct inode *inode)
{
    printk(KERN_INFO "SimpleFS: Evict inode %lu bat dau\n", inode->i_ino);
    truncate_inode_pages_final(&inode->i_data);
    clear_inode(inode);
    printk(KERN_INFO "SimpleFS: Evict inode %lu ket thuc\n", inode->i_ino);
}

/* Hàm xử lý khi put superblock */
static void simplefs_put_super(struct super_block *sb)
{
    printk(KERN_INFO "SimpleFS: Giai phong superblock\n");
}

/* Super operations - gán simplefs_drop_inode và put_super vào struct */
static const struct super_operations simplefs_super_ops = {
    .statfs = simplefs_statfs,
    .drop_inode = simplefs_drop_inode,
    .evict_inode = simplefs_evict_inode,
    .put_super = simplefs_put_super,
};

/* Hàm điền thông tin superblock */
static int simplefs_fill_super(struct super_block *sb, void *data, int silent)
{
    struct inode *root_inode;
    struct dentry *root_dentry;

    printk(KERN_INFO "SimpleFS: Dien thong tin superblock\n");

    sb->s_magic = SIMPLEFS_MAGIC;
    sb->s_op = &simplefs_super_ops;
    sb->s_time_gran = 1;
    sb->s_maxbytes = MAX_LFS_FILESIZE;
    sb->s_blocksize = PAGE_SIZE;
    sb->s_blocksize_bits = PAGE_SHIFT;

    /* Truyền i_ino mong muốn NGAY TỪ ĐẦU, không gán lại sau khi
     * inode đã nằm trong cấu trúc quản lý của superblock */
    root_inode = simplefs_get_inode(sb, NULL, S_IFDIR | SIMPLEFS_DEFAULT_MODE,
                                     SIMPLEFS_ROOT_INO, 0);
    if (!root_inode) {
        printk(KERN_ALERT "SimpleFS: Khong the tao root inode\n");
        return -ENOMEM;
    }

    root_dentry = d_make_root(root_inode);
    if (!root_dentry) {
        printk(KERN_ALERT "SimpleFS: Khong the tao root dentry\n");
        iput(root_inode);
        return -ENOMEM;
    }

    sb->s_root = root_dentry;

    printk(KERN_INFO "SimpleFS: Khoi tao superblock thanh cong\n");

    return 0;
}

/* Hàm mount filesystem */
static struct dentry *simplefs_mount(struct file_system_type *fs_type,
                                    int flags,
                                    const char *dev_name,
                                    void *data)
{
    struct dentry *ret;

    printk(KERN_INFO "SimpleFS: Mount filesystem, dev_name=%s\n",
           dev_name ? dev_name : "none");

    ret = mount_nodev(fs_type, flags, data, simplefs_fill_super);

    if (IS_ERR(ret))
        printk(KERN_ALERT "SimpleFS: Mount that bai voi loi %ld\n", PTR_ERR(ret));
    else
        printk(KERN_INFO "SimpleFS: Mount thanh cong\n");

    return ret;
}

/* Hàm kill superblock khi umount */
static void simplefs_kill_sb(struct super_block *sb)
{
    printk(KERN_INFO "SimpleFS: Huy superblock bat dau\n");
    kill_litter_super(sb);
    printk(KERN_INFO "SimpleFS: Filesystem da unmount thanh cong\n");
}

/* Cấu trúc filesystem type */
static struct file_system_type simplefs_fs_type = {
    .owner = THIS_MODULE,
    .name = "simplefs",
    .mount = simplefs_mount,
    .kill_sb = simplefs_kill_sb,
    .fs_flags = 0,
};

/* Hàm khởi tạo module */
static int __init simplefs_init(void)
{
    int ret;

    printk(KERN_INFO "SimpleFS: Khoi tao module\n");

    ret = register_filesystem(&simplefs_fs_type);
    if (ret) {
        printk(KERN_ALERT "SimpleFS: Dang ky filesystem that bai, loi %d\n", ret);
        return ret;
    }

    printk(KERN_INFO "SimpleFS: Load module thanh cong\n");
    printk(KERN_INFO "SimpleFS: Co the mount voi lenh: mount -t simplefs none /mnt/point\n");

    return 0;
}

/* Hàm cleanup module */
static void __exit simplefs_exit(void)
{
    int ret;

    printk(KERN_INFO "SimpleFS: Gỡ bỏ module\n");

    ret = unregister_filesystem(&simplefs_fs_type);
    if (ret)
        printk(KERN_ALERT "SimpleFS: Huy dang ky filesystem that bai, loi %d\n", ret);
    else
        printk(KERN_INFO "SimpleFS: Go module thanh cong\n");
}

module_init(simplefs_init);
module_exit(simplefs_exit);