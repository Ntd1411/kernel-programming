/*
 * simplefs.c - Simple Virtual Filesystem Kernel Module
 *
 * Build: make
 * Load: sudo insmod simplefs.ko
 * Mount: sudo mount -t simplefs none /mnt/simplefs
 * Umount: sudo umount /mnt/simplefs
 * Unload: sudo rmmod simplefs
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mnt_idmapping.h>
#include <linux/statfs.h>
#include <linux/pagemap.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("VFS Learning");
MODULE_DESCRIPTION("Simple VFS Module - Learning 4 Structs");
MODULE_VERSION("2.0");

#define vfs_module_MAGIC 0xDEADBEEF

/* ============================================================================
 * ĐỊNH NGHĨA INODE (metadata file/directory)
 * ============================================================================
 */

/* Định danh inode */
enum {
    ROOT_INO = 1,
    HELLO_INO = 2,
};

/* Nội dung file ảo */
static const char hello_content[] = "Hello from VFS Demo!\nThis file lives in kernel memory.\n";

/* ============================================================================
 * INTERFACE: file_operations - các thao tác trên file handle (struct file)
 * ============================================================================
 */

/**
 * vfs_module_read() - Đọc dữ liệu từ file
 *
 * Được gọi khi user space làm: read(fd, buf, count)
 * Tham số:
 *   filp  - file handle (struct file) mà user nắm giữ
 *   buf   - user space buffer (unsafe, cần copy_to_user)
 *   len   - số byte muốn đọc
 *   ppos  - vị trí hiện tại trong file (offset)
 *
 * Trả về: số byte đã đọc, hoặc lỗi < 0
 */
static ssize_t vfs_module_read(struct file *filp, char __user *buf,
                            size_t len, loff_t *ppos)
{
    struct inode *inode = file_inode(filp);
    const char *content;
    size_t content_len;
    size_t to_copy;

    pr_info("vfs_module: READ inode#%lu, offset=%lld, len=%zu\n",
            inode->i_ino, *ppos, len);

    /* Tìm nội dung file dựa vào inode number */
    if (inode->i_ino == HELLO_INO) {
        content = hello_content;
        content_len = sizeof(hello_content) - 1; /* trừ null terminator */
    } else {
        pr_err("vfs_module: invalid inode %lu for read\n", inode->i_ino);
        return -EINVAL;
    }

    /* Kiểm tra: đã đọc hết file? */
    if (*ppos >= content_len)
        return 0;  /* EOF */

    /* Tính số byte cần copy (không vượt quá: file size - offset) */
    to_copy = min(len, content_len - (size_t)*ppos);

    /* Copy từ kernel space sang user space (an toàn) */
    if (copy_to_user(buf, content + *ppos, to_copy))
        return -EFAULT;  /* lỗi copy_to_user */

    /* Cập nhật offset */
    *ppos += to_copy;

    pr_info("vfs_module: READ returned %zu bytes\n", to_copy);

    return to_copy;
}

/**
 * vfs_module_open() - Mở file (optional, gọi khi open())
 *
 * Trong ví dụ này, không cần làm gì đặc biệt.
 */
static int vfs_module_open(struct inode *inode, struct file *filp)
{
    pr_info("vfs_module: OPEN inode#%lu\n", inode->i_ino);
    return 0;
}

/**
 * vfs_module_release() - Đóng file (optional, gọi khi close())
 *
 * Dùng để dọn dẹp nếu có resource được allocate ở open().
 */
static int vfs_module_release(struct inode *inode, struct file *filp)
{
    pr_info("vfs_module: CLOSE inode#%lu\n", inode->i_ino);
    return 0;
}

/* Struct file_operations - định nghĩa các thao tác có thể làm trên file handle */
static const struct file_operations vfs_module_file_ops = {
    .read    = vfs_module_read,
    .open    = vfs_module_open,
    .release = vfs_module_release,
    .llseek  = generic_file_llseek,  /* Cho phép seek (lseek syscall) */
};

/* ============================================================================
 * INTERFACE: inode_operations - các thao tác liên quan inode (lookup, etc)
 * ============================================================================
 */

/**
 * vfs_module_lookup() - Tìm file bằng tên trong directory
 *
 * Được gọi khi user space làm: open("/mnt/vfs_module/hello")
 * VFS parse path: "/" → lookup trong root inode
 *                 "hello" → lookup trong root, tìm dentry "hello"
 *
 * Nếu dentry chưa có trong cache → gọi inode->i_op->lookup
 *
 * Tham số:
 *   dir     - inode của directory cha (root inode trong trường hợp này)
 *   dentry  - dentry chưa được link với inode nào (chúa là "negative dentry")
 *   flags   - cờ lookup (bỏ qua ở đây)
 *
 * Trả về: NULL nếu tìm được và gọi d_add(), hoặc ERR_PTR() nếu lỗi
 */
static struct dentry *vfs_module_lookup(struct inode *dir, struct dentry *dentry,
                                     unsigned int flags)
{
    struct inode *inode = NULL;

    pr_info("vfs_module: LOOKUP '%s' in directory inode#%lu\n",
            dentry->d_name.name, dir->i_ino);

    /* Chỉ hỗ trợ lookup trong root directory */
    if (dir->i_ino != ROOT_INO) {
        pr_info("vfs_module: lookup not in root, return ENOENT\n");
        return ERR_PTR(-ENOENT);
    }

    /* Kiểm tra tên file */
    if (strcmp(dentry->d_name.name, "hello") == 0) {
        /* Nếu tìm được, tạo inode struct cho file "hello" */
        inode = new_inode(dir->i_sb);
        if (!inode)
            return ERR_PTR(-ENOMEM);

        /* Khởi tạo inode: số hiệu, quyền, loại file, operations */
        inode->i_ino = HELLO_INO;
        inode->i_mode = S_IFREG | 0444;  /* Regular file, read-only */
        inode->i_uid = GLOBAL_ROOT_UID;
        inode->i_gid = GLOBAL_ROOT_GID;
        set_nlink(inode, 1);

        /* Set thời gian tạo */
        simple_inode_init_ts(inode);

        /* Gán operations cho inode */
        inode->i_op = &simple_dir_inode_operations;  /* Không cần lookup nữa */
        inode->i_fop = &vfs_module_file_ops;           /* Cho phép read */

        /* Set kích thước file (VFS dùng để kiểm tra seek) */
        i_size_write(inode, sizeof(hello_content) - 1);

        pr_info("vfs_module: Created inode#%lu for 'hello'\n", inode->i_ino);
    } else {
        pr_info("vfs_module: file '%s' not found\n", dentry->d_name.name);
        /* File không tồn tại → negative dentry (inode = NULL) */
    }

    /* Link dentry với inode (nếu tìm được) hoặc tạo negative dentry (NULL) */
    d_add(dentry, inode);

    return NULL;  /* Trả về NULL có nghĩa: thành công, dentry đã được link */
}

/**
 * vfs_module_iterate_shared() - Liệt kê các file trong directory (ls)
 *
 * Được gọi khi user space làm: ls /mnt/vfs_module hoặc readdir()
 *
 * Tham số:
 *   file  - file handle của directory
 *   ctx   - context để emit các entry (dir_emit helper)
 *
 * Trả về: 0 = thành công, <0 = lỗi
 */
static int vfs_module_iterate_shared(struct file *file, struct dir_context *ctx)
{
    struct inode *inode = file_inode(file);

    pr_info("vfs_module: ITERATE directory inode#%lu, pos=%lld\n",
            inode->i_ino, ctx->pos);

    /* Chỉ hỗ trợ iterate trên root directory */
    if (inode->i_ino != ROOT_INO)
        return -ENOENT;

    /* Emit . và .. (parent directory) - helper VFS */
    if (!dir_emit_dots(file, ctx))
        return 0;

    /* Emit entry "hello" nếu chưa emit */
    if (ctx->pos == 2) {
        dir_emit(ctx, "hello", 5, HELLO_INO, DT_REG);
        ctx->pos++;
    }

    return 0;
}

/* Struct inode_operations - định nghĩa các thao tác liên quan inode */
static const struct inode_operations vfs_module_dir_inode_ops = {
    .lookup = vfs_module_lookup,
};

/* Struct file_operations cho directory (dùng trong iterate) */
static const struct file_operations vfs_module_dir_ops = {
    .iterate_shared = vfs_module_iterate_shared,
    .llseek = default_llseek,
};

/* ============================================================================
 * INTERFACE: super_operations - các thao tác liên quan superblock
 * ============================================================================
 */

/**
 * vfs_module_evict_inode() - Gọi khi inode được giải phóng
 *
 * VFS gọi hàm này khi count của inode về 0 (không ai nắm giữ nữa)
 * Dùng để dọn dẹp any custom state gắn vào inode.
 */
static void vfs_module_evict_inode(struct inode *inode)
{
    pr_info("vfs_module: EVICT inode#%lu\n", inode->i_ino);

    /* Truncate data (nếu có) */
    truncate_inode_pages_final(&inode->i_data);

    /* Clear inode (cleanup) */
    clear_inode(inode);
}

/**
 * vfs_module_statfs() - Lấy thông tin filesystem (df command)
 *
 * Tham số:
 *   dentry  - dentry của mount point
 *   buf     - buffer để fill thông tin
 *
 * Trả về: 0 = thành công
 */
static int vfs_module_statfs(struct dentry *dentry, struct kstatfs *buf)
{
    pr_info("vfs_module: STATFS called\n");

    buf->f_type = vfs_module_MAGIC;
    buf->f_bsize = PAGE_SIZE;
    buf->f_namelen = 255;
    buf->f_blocks = 0;    /* Filesystem này không có "blocks" thực */
    buf->f_bfree = 0;
    buf->f_bavail = 0;
    buf->f_files = 2;     /* Chỉ có 2 inode: root + hello */
    buf->f_ffree = 0;

    return 0;
}

/* Struct super_operations - định nghĩa các thao tác liên quan superblock */
static const struct super_operations vfs_module_super_ops = {
    .evict_inode = vfs_module_evict_inode,
    .statfs = vfs_module_statfs,
};

/* ============================================================================
 * FILL SUPERBLOCK - Khởi tạo filesystem
 * ============================================================================
 */

/**
 * vfs_module_fill_super() - Gọi lần đầu khi mount filesystem
 *
 * Tham số:
 *   sb    - superblock đã được VFS tạo trước
 *   data  - mount options từ user space
 *   silent - có nên in lỗi không
 *
 * Trả về: 0 = thành công, <0 = lỗi
 */
static int vfs_module_fill_super(struct super_block *sb, void *data, int silent)
{
    struct inode *root_inode;
    struct dentry *root_dentry;

    pr_info("vfs_module: FILL_SUPER - initializing filesystem\n");

    /* Cấu hình superblock */
    sb->s_magic = vfs_module_MAGIC;
    sb->s_op = &vfs_module_super_ops;
    sb->s_time_gran = 1;          /* 1 second time granularity */
    sb->s_maxbytes = MAX_LFS_FILESIZE;
    sb->s_blocksize = PAGE_SIZE;
    sb->s_blocksize_bits = PAGE_SHIFT;

    pr_info("vfs_module: superblock configured (magic=0x%x, blocksize=%lu)\n",
            vfs_module_MAGIC, PAGE_SIZE);

    /* Tạo root inode */
    root_inode = new_inode(sb);
    if (!root_inode) {
        pr_err("vfs_module: failed to create root inode\n");
        return -ENOMEM;
    }

    /* Khởi tạo root inode: directory, quyền 755 */
    root_inode->i_ino = ROOT_INO;
    root_inode->i_mode = S_IFDIR | 0755;
    root_inode->i_uid = GLOBAL_ROOT_UID;
    root_inode->i_gid = GLOBAL_ROOT_GID;
    set_nlink(root_inode, 2);     /* . và .. */

    /* Set thời gian */
    simple_inode_init_ts(root_inode);

    /* Gán operations cho root inode */
    root_inode->i_op = &vfs_module_dir_inode_ops;  /* Có lookup */
    root_inode->i_fop = &vfs_module_dir_ops;       /* Có iterate */

    pr_info("vfs_module: root inode created (ino=%lu, mode=dir)\n",
            root_inode->i_ino);

    /* Tạo root dentry (liên kết tên "/" ↔ root inode) */
    root_dentry = d_make_root(root_inode);
    if (!root_dentry) {
        pr_err("vfs_module: failed to create root dentry\n");
        iput(root_inode);
        return -ENOMEM;
    }

    sb->s_root = root_dentry;

    pr_info("vfs_module: root dentry created, filesystem ready\n");

    return 0;
}

/* ============================================================================
 * MOUNT / UMOUNT
 * ============================================================================
 */

/**
 * vfs_module_mount() - Gọi khi user space làm: mount -t vfs_module none /mnt/...
 *
 * VFS gọi hàm này để gọi fill_super().
 */
static struct dentry *vfs_module_mount(struct file_system_type *fs_type,
                                    int flags,
                                    const char *dev_name,
                                    void *data)
{
    pr_info("vfs_module: MOUNT called (dev_name=%s)\n",
            dev_name ? dev_name : "(none)");

    /* mount_nodev: filesystem này không cần backing device thực */
    return mount_nodev(fs_type, flags, data, vfs_module_fill_super);
}

/**
 * vfs_module_kill_sb() - Gọi khi user space làm: umount /mnt/...
 *
 * Dọn dẹp superblock, giải phóng dentry cache, inode, v.v.
 */
static void vfs_module_kill_sb(struct super_block *sb)
{
    pr_info("vfs_module: KILL_SB - unmounting filesystem (BEGIN)\n");

    /* kill_anon_super: dùng cho anonymous super_block (không có backing device)
     * Nó sẽ:
     *   - gọi generic_shutdown_super() → shrink_dcache_for_umount
     *   - gọi deactivate_locked_super()
     *   - dọn dẹp dentry cache, inode hash
     */
    kill_anon_super(sb);

    pr_info("vfs_module: KILL_SB - filesystem unmounted (END)\n");
}

/**
 * file_system_type - Đăng ký filesystem type với kernel
 */
static struct file_system_type vfs_module_fs_type = {
    .owner = THIS_MODULE,
    .name = "vfs_module",
    .mount = vfs_module_mount,
    .kill_sb = vfs_module_kill_sb,
};

/* ============================================================================
 * MODULE INIT / EXIT
 * ============================================================================
 */

static int __init vfs_module_init(void)
{
    int ret;

    pr_info("vfs_module: ========== MODULE INIT (START) ==========\n");

    /* Đăng ký filesystem type với kernel */
    ret = register_filesystem(&vfs_module_fs_type);
    if (ret) {
        pr_err("vfs_module: register_filesystem failed (ret=%d)\n", ret);
        return ret;
    }

    pr_info("vfs_module: filesystem 'vfs_module' registered\n");
    pr_info("vfs_module: ========== MODULE INIT (END) ==========\n");
    pr_info("vfs_module: Usage:\n");
    pr_info("vfs_module:   mount -t vfs_module none /mnt/vfs_module\n");
    pr_info("vfs_module:   cat /mnt/vfs_module/hello\n");
    pr_info("vfs_module:   ls /mnt/vfs_module\n");
    pr_info("vfs_module:   umount /mnt/vfs_module\n");

    return 0;
}

static void __exit vfs_module_exit(void)
{
    pr_info("vfs_module: ========== MODULE EXIT (START) ==========\n");

    unregister_filesystem(&vfs_module_fs_type);

    pr_info("vfs_module: filesystem 'vfs_module' unregistered\n");
    pr_info("vfs_module: ========== MODULE EXIT (END) ==========\n");
}

module_init(vfs_module_init);
module_exit(vfs_module_exit);
