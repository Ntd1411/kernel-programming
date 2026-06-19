/*
 * vfs_module.c - Simple Virtual Filesystem Kernel Module
 * 
 * Tạo một filesystem đơn giản có thể mount với các file ảo
 * 
 * Build: make
 * Load: sudo insmod vfs_module.ko
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
MODULE_AUTHOR("VFS Learning");
MODULE_DESCRIPTION("Simple Virtual Filesystem Module");
MODULE_VERSION("1.0");

#define SIMPLEFS_MAGIC 0x19920342
#define SIMPLEFS_DEFAULT_MODE 0755

/* Noi dung cac file ao */
static const char hello_content[] = "Hello from SimplFS!\nThis is a virtual file in kernel space.\n";
static const char info_content[] = "SimplFS Information:\n"
                                   "- Type: Virtual Filesystem\n"
                                   "- Mode: Read-only\n"
                                   "- Storage: Memory only\n"
                                   "- Purpose: Learning and demonstration\n";

/* Inode numbers */
enum {
    SIMPLEFS_ROOT_INO = 1,
    SIMPLEFS_HELLO_INO = 2,
    SIMPLEFS_INFO_INO = 3,
};

/* Forward declarations */
static struct inode *simplefs_get_inode(struct super_block *sb, 
                                        const struct inode *dir,
                                        umode_t mode, 
                                        dev_t dev);

/* Dinh nghia cac operations */
static const struct inode_operations simplefs_dir_inode_operations;
static const struct file_operations simplefs_file_operations;
static const struct super_operations simplefs_super_ops;

/*
 * simplefs_read - Doc noi dung file ao
 */
static ssize_t simplefs_read(struct file *filp, char __user *buf, 
                            size_t len, loff_t *ppos)
{
    struct inode *inode = file_inode(filp);
    const char *content = NULL;
    size_t content_len = 0;
    size_t to_copy;
    
    pr_info("simplefs: read called for inode %lu, pos %lld, len %zu\n",
            inode->i_ino, *ppos, len);
    
    /* Xac dinh noi dung dua vao inode number */
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
    
    /* Kiem tra offset */
    if (*ppos >= content_len)
        return 0;
    
    /* Tinh so byte can copy */
    to_copy = min(len, content_len - (size_t)*ppos);
    
    /* Copy du lieu tu kernel space sang user space */
    if (copy_to_user(buf, content + *ppos, to_copy))
        return -EFAULT;
    
    *ppos += to_copy;
    
    pr_info("simplefs: read %zu bytes from inode %lu\n", to_copy, inode->i_ino);
    
    return to_copy;
}

/*
 * simplefs_file_operations - Cac thao tac voi file
 */
static const struct file_operations simplefs_file_operations = {
    .read = simplefs_read,
    .llseek = generic_file_llseek,
};

/*
 * simplefs_lookup - Tim inode trong directory
 */
static struct dentry *simplefs_lookup(struct inode *dir, struct dentry *dentry,
                                     unsigned int flags)
{
    struct inode *inode = NULL;
    
    pr_info("simplefs: lookup '%s' in directory inode %lu\n", 
            dentry->d_name.name, dir->i_ino);
    
    /* Chi ho tro lookup trong root directory */
    if (dir->i_ino != SIMPLEFS_ROOT_INO)
        goto out;
    
    /* Tim file theo ten */
    if (strcmp(dentry->d_name.name, "hello") == 0) {
        inode = simplefs_get_inode(dir->i_sb, dir, S_IFREG | 0644, 0);
        if (inode)
            inode->i_ino = SIMPLEFS_HELLO_INO;
    } else if (strcmp(dentry->d_name.name, "info") == 0) {
        inode = simplefs_get_inode(dir->i_sb, dir, S_IFREG | 0644, 0);
        if (inode)
            inode->i_ino = SIMPLEFS_INFO_INO;
    }
    
out:
    /* Gan inode vao dentry - NULL inode nghia la file khong ton tai */
    d_add(dentry, inode);
    return NULL;
}

/*
 * simplefs_iterate - Liet ke cac file trong directory
 */
static int simplefs_iterate(struct file *file, struct dir_context *ctx)
{
    struct inode *inode = file_inode(file);
    
    pr_info("simplefs: iterate directory inode %lu, pos %lld\n", 
            inode->i_ino, ctx->pos);
    
    /* Chi ho tro iterate root directory */
    if (inode->i_ino != SIMPLEFS_ROOT_INO)
        return -ENOENT;
    
    /* Emit . va .. */
    if (!dir_emit_dots(file, ctx))
        return 0;
    
    /* Emit 'hello' file */
    if (ctx->pos == 2) {
        if (!dir_emit(ctx, "hello", 5, SIMPLEFS_HELLO_INO, DT_REG))
            return 0;
        ctx->pos++;
    }
    
    /* Emit 'info' file */
    if (ctx->pos == 3) {
        if (!dir_emit(ctx, "info", 4, SIMPLEFS_INFO_INO, DT_REG))
            return 0;
        ctx->pos++;
    }
    
    return 0;
}

/*
 * simplefs_dir_operations - Cac thao tac voi directory
 */
static const struct file_operations simplefs_dir_operations = {
    .read = generic_read_dir,
    .iterate_shared = simplefs_iterate,
    .llseek = generic_file_llseek,
};

/*
 * simplefs_dir_inode_operations - Inode operations cho directory
 */
static const struct inode_operations simplefs_dir_inode_operations = {
    .lookup = simplefs_lookup,
};

/*
 * simplefs_get_inode - Tao va khoi tao inode moi
 */
static struct inode *simplefs_get_inode(struct super_block *sb,
                                       const struct inode *dir,
                                       umode_t mode,
                                       dev_t dev)
{
    struct inode *inode = new_inode(sb);
    
    if (!inode)
        return NULL;
    
    /* Luu y: inode number se duoc gan sau boi caller neu can */
    inode->i_ino = get_next_ino();
    inode_init_owner(&nop_mnt_idmap, inode, dir, mode);
    
    /* Set timestamps */
    simple_inode_init_ts(inode);
    
    switch (mode & S_IFMT) {
        case S_IFDIR:
            /* Directory inode */
            inode->i_op = &simplefs_dir_inode_operations;
            inode->i_fop = &simplefs_dir_operations;
            inc_nlink(inode);
            break;
        case S_IFREG:
            /* Regular file inode */
            inode->i_op = &simple_dir_inode_operations;
            inode->i_fop = &simplefs_file_operations;
            break;
        default:
            init_special_inode(inode, mode, dev);
            break;
    }
    
    pr_info("simplefs: created inode %lu with mode 0%o\n", inode->i_ino, mode);
    
    return inode;
}

/*
 * simplefs_statfs - Lay thong tin filesystem
 */
static int simplefs_statfs(struct dentry *dentry, struct kstatfs *buf)
{
    pr_info("simplefs: statfs called\n");
    
    buf->f_type = SIMPLEFS_MAGIC;
    buf->f_bsize = PAGE_SIZE;
    buf->f_namelen = 255;
    buf->f_blocks = 0;
    buf->f_bfree = 0;
    buf->f_bavail = 0;
    buf->f_files = 3; /* root, hello, info */
    buf->f_ffree = 0;
    
    return 0;
}

/*
 * simplefs_drop_inode - Xu ly khi inode khong con su dung
 */
static int simplefs_drop_inode(struct inode *inode)
{
    pr_info("simplefs: dropping inode %lu\n", inode->i_ino);
    return generic_drop_inode(inode);
}

/*
 * simplefs_evict_inode - Giai phong inode hoan toan
 */
static void simplefs_evict_inode(struct inode *inode)
{
    pr_info("simplefs: evicting inode %lu\n", inode->i_ino);
    truncate_inode_pages_final(&inode->i_data);
    clear_inode(inode);
}

/*
 * simplefs_super_operations - Cac thao tac voi superblock
 */
static const struct super_operations simplefs_super_ops = {
    .statfs = simplefs_statfs,
    .drop_inode = simplefs_drop_inode,
    .evict_inode = simplefs_evict_inode,
};

/*
 * simplefs_fill_super - Khoi tao superblock va root inode
 */
static int simplefs_fill_super(struct super_block *sb, void *data, int silent)
{
    struct inode *root_inode;
    struct dentry *root_dentry;
    
    pr_info("simplefs: filling superblock\n");
    
    /* Setup superblock */
    sb->s_magic = SIMPLEFS_MAGIC;
    sb->s_op = &simplefs_super_ops;
    sb->s_time_gran = 1;
    sb->s_maxbytes = MAX_LFS_FILESIZE;
    sb->s_blocksize = PAGE_SIZE;
    sb->s_blocksize_bits = PAGE_SHIFT;
    
    /* Tao root inode */
    root_inode = simplefs_get_inode(sb, NULL, S_IFDIR | SIMPLEFS_DEFAULT_MODE, 0);
    if (!root_inode) {
        pr_err("simplefs: failed to create root inode\n");
        return -ENOMEM;
    }
    
    root_inode->i_ino = SIMPLEFS_ROOT_INO;
    
    /* Tao root dentry */
    root_dentry = d_make_root(root_inode);
    if (!root_dentry) {
        pr_err("simplefs: failed to create root dentry\n");
        return -ENOMEM;
    }
    
    sb->s_root = root_dentry;
    
    pr_info("simplefs: superblock initialized successfully\n");
    
    return 0;
}

/*
 * simplefs_mount - Mount filesystem
 */
static struct dentry *simplefs_mount(struct file_system_type *fs_type,
                                    int flags,
                                    const char *dev_name,
                                    void *data)
{
    struct dentry *ret;
    
    pr_info("simplefs: mounting filesystem, dev_name=%s\n", 
            dev_name ? dev_name : "none");
    
    ret = mount_nodev(fs_type, flags, data, simplefs_fill_super);
    
    if (IS_ERR(ret))
        pr_err("simplefs: mount failed with error %ld\n", PTR_ERR(ret));
    else
        pr_info("simplefs: mount successful\n");
    
    return ret;
}

/*
 * simplefs_kill_sb - Unmount filesystem
 */
static void simplefs_kill_sb(struct super_block *sb)
{
    pr_info("simplefs: killing superblock\n");
    kill_litter_super(sb);
    pr_info("simplefs: filesystem unmounted\n");
}

/*
 * simplefs_fs_type - Dinh nghia filesystem type
 */
static struct file_system_type simplefs_fs_type = {
    .owner = THIS_MODULE,
    .name = "simplefs",
    .mount = simplefs_mount,
    .kill_sb = simplefs_kill_sb,
    .fs_flags = 0,
};

/*
 * simplefs_init - Khoi tao module
 */
static int __init simplefs_init(void)
{
    int ret;
    
    pr_info("simplefs: initializing module\n");
    
    ret = register_filesystem(&simplefs_fs_type);
    if (ret) {
        pr_err("simplefs: failed to register filesystem, error %d\n", ret);
        return ret;
    }
    
    pr_info("simplefs: module loaded successfully\n");
    pr_info("simplefs: you can now mount with: mount -t simplefs none /mnt/point\n");
    
    return 0;
}

/*
 * simplefs_exit - Cleanup module
 */
static void __exit simplefs_exit(void)
{
    int ret;
    
    pr_info("simplefs: unloading module\n");
    
    ret = unregister_filesystem(&simplefs_fs_type);
    if (ret)
        pr_err("simplefs: failed to unregister filesystem, error %d\n", ret);
    else
        pr_info("simplefs: module unloaded successfully\n");
}

module_init(simplefs_init);
module_exit(simplefs_exit);