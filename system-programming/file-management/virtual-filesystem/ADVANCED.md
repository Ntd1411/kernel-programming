# Hướng dẫn mở rộng SimplFS

## Giới thiệu

File này chứa các hướng dẫn để mở rộng SimplFS với các tính năng nâng cao hơn.

## 1. Thêm hỗ trợ Write Operations

Để cho phép ghi file, bạn cần:

### Bước 1: Tạo cấu trúc lưu trữ nội dung động

```c
/* Them vao dau file */
struct simplefs_file_data {
    char *content;
    size_t size;
    size_t capacity;
};

static struct simplefs_file_data *file_data[256];
static DEFINE_MUTEX(file_data_lock);
```

### Bước 2: Implement hàm write

```c
static ssize_t simplefs_write(struct file *filp, const char __user *buf,
                             size_t len, loff_t *ppos)
{
    struct inode *inode = file_inode(filp);
    struct simplefs_file_data *data;
    char *new_content;
    size_t new_size;
    
    mutex_lock(&file_data_lock);
    
    /* Lay hoac tao file data */
    data = file_data[inode->i_ino];
    if (!data) {
        data = kzalloc(sizeof(*data), GFP_KERNEL);
        if (!data) {
            mutex_unlock(&file_data_lock);
            return -ENOMEM;
        }
        file_data[inode->i_ino] = data;
    }
    
    /* Tinh kich thuoc moi */
    new_size = max((size_t)(*ppos + len), data->size);
    
    /* Reallocate neu can */
    if (new_size > data->capacity) {
        new_content = krealloc(data->content, new_size, GFP_KERNEL);
        if (!new_content) {
            mutex_unlock(&file_data_lock);
            return -ENOMEM;
        }
        data->content = new_content;
        data->capacity = new_size;
    }
    
    /* Copy du lieu tu user space */
    if (copy_from_user(data->content + *ppos, buf, len)) {
        mutex_unlock(&file_data_lock);
        return -EFAULT;
    }
    
    data->size = new_size;
    *ppos += len;
    inode->i_size = data->size;
    
    mutex_unlock(&file_data_lock);
    
    return len;
}
```

### Bước 3: Cập nhật file_operations

```c
static const struct file_operations simplefs_file_operations = {
    .read = simplefs_read,
    .write = simplefs_write,  /* Them dong nay */
    .llseek = generic_file_llseek,
};
```

## 2. Thêm hỗ trợ tạo file mới (create)

### Bước 1: Implement create operation

```c
static int simplefs_create(struct inode *dir, struct dentry *dentry,
                          umode_t mode, bool excl)
{
    struct inode *inode;
    
    pr_info("simplefs: creating file '%s'\n", dentry->d_name.name);
    
    /* Tao inode moi cho file */
    inode = simplefs_get_inode(dir->i_sb, dir, S_IFREG | mode, 0);
    if (!inode)
        return -ENOSPC;
    
    /* Gan inode vao dentry */
    d_instantiate(dentry, inode);
    dget(dentry);
    
    dir->i_mtime = dir->i_ctime = current_time(dir);
    
    return 0;
}
```

### Bước 2: Cập nhật dir_inode_operations

```c
static const struct inode_operations simplefs_dir_inode_operations = {
    .lookup = simplefs_lookup,
    .create = simplefs_create,  /* Them dong nay */
};
```

## 3. Thêm hỗ trợ tạo thư mục (mkdir)

```c
static int simplefs_mkdir(struct inode *dir, struct dentry *dentry,
                         umode_t mode)
{
    struct inode *inode;
    
    pr_info("simplefs: creating directory '%s'\n", dentry->d_name.name);
    
    /* Tang link count cua parent directory */
    inc_nlink(dir);
    
    /* Tao inode cho directory */
    inode = simplefs_get_inode(dir->i_sb, dir, S_IFDIR | mode, 0);
    if (!inode) {
        drop_nlink(dir);
        return -ENOSPC;
    }
    
    d_instantiate(dentry, inode);
    dget(dentry);
    
    return 0;
}

/* Them vao dir_inode_operations */
static const struct inode_operations simplefs_dir_inode_operations = {
    .lookup = simplefs_lookup,
    .create = simplefs_create,
    .mkdir = simplefs_mkdir,  /* Them dong nay */
};
```

## 4. Thêm hỗ trợ xóa file (unlink)

```c
static int simplefs_unlink(struct inode *dir, struct dentry *dentry)
{
    struct inode *inode = d_inode(dentry);
    
    pr_info("simplefs: unlinking file '%s'\n", dentry->d_name.name);
    
    /* Giam link count */
    drop_nlink(inode);
    
    /* Cap nhat timestamps */
    dir->i_mtime = dir->i_ctime = current_time(dir);
    
    return 0;
}

/* Them vao dir_inode_operations */
static const struct inode_operations simplefs_dir_inode_operations = {
    .lookup = simplefs_lookup,
    .create = simplefs_create,
    .mkdir = simplefs_mkdir,
    .unlink = simplefs_unlink,  /* Them dong nay */
};
```

## 5. Thêm mount options

### Bước 1: Định nghĩa mount options

```c
enum {
    Opt_mode,
    Opt_uid,
    Opt_gid,
    Opt_err
};

static const match_table_t tokens = {
    {Opt_mode, "mode=%o"},
    {Opt_uid, "uid=%u"},
    {Opt_gid, "gid=%u"},
    {Opt_err, NULL}
};

struct simplefs_mount_opts {
    umode_t mode;
    kuid_t uid;
    kgid_t gid;
};
```

### Bước 2: Parse mount options

```c
static int simplefs_parse_options(char *data, 
                                 struct simplefs_mount_opts *opts)
{
    substring_t args[MAX_OPT_ARGS];
    int option;
    int token;
    char *p;
    
    /* Default values */
    opts->mode = SIMPLEFS_DEFAULT_MODE;
    opts->uid = current_fsuid();
    opts->gid = current_fsgid();
    
    if (!data)
        return 0;
    
    while ((p = strsep(&data, ",")) != NULL) {
        if (!*p)
            continue;
        
        token = match_token(p, tokens, args);
        switch (token) {
            case Opt_mode:
                if (match_octal(&args[0], &option))
                    return -EINVAL;
                opts->mode = option & 0777;
                break;
            case Opt_uid:
                if (match_int(&args[0], &option))
                    return -EINVAL;
                opts->uid = make_kuid(current_user_ns(), option);
                break;
            case Opt_gid:
                if (match_int(&args[0], &option))
                    return -EINVAL;
                opts->gid = make_kgid(current_user_ns(), option);
                break;
            default:
                pr_err("simplefs: unrecognized mount option '%s'\n", p);
                return -EINVAL;
        }
    }
    
    return 0;
}
```

### Bước 3: Sử dụng trong fill_super

```c
static int simplefs_fill_super(struct super_block *sb, void *data, 
                              int silent)
{
    struct simplefs_mount_opts opts;
    int ret;
    
    /* Parse mount options */
    ret = simplefs_parse_options(data, &opts);
    if (ret)
        return ret;
    
    /* Su dung opts.mode, opts.uid, opts.gid khi tao files */
    /* ... */
}
```

## 6. Thêm Extended Attributes (xattr)

```c
static int simplefs_setxattr(struct dentry *dentry, struct inode *inode,
                            const char *name, const void *value,
                            size_t size, int flags)
{
    /* Implementation */
    pr_info("simplefs: setxattr %s\n", name);
    return -EOPNOTSUPP;
}

static ssize_t simplefs_getxattr(struct dentry *dentry, struct inode *inode,
                                const char *name, void *buffer, size_t size)
{
    /* Implementation */
    pr_info("simplefs: getxattr %s\n", name);
    return -EOPNOTSUPP;
}

/* Them vao inode_operations */
static const struct inode_operations simplefs_inode_operations = {
    /* ... cac operations khac ... */
    .setxattr = simplefs_setxattr,
    .getxattr = simplefs_getxattr,
};
```

## 7. Thêm Symbolic Links

```c
static int simplefs_symlink(struct inode *dir, struct dentry *dentry,
                           const char *symname)
{
    struct inode *inode;
    int len = strlen(symname) + 1;
    char *link;
    
    pr_info("simplefs: creating symlink '%s' -> '%s'\n",
            dentry->d_name.name, symname);
    
    /* Allocate inode */
    inode = simplefs_get_inode(dir->i_sb, dir, S_IFLNK | 0777, 0);
    if (!inode)
        return -ENOSPC;
    
    /* Allocate va copy symlink target */
    link = kmalloc(len, GFP_KERNEL);
    if (!link) {
        iput(inode);
        return -ENOMEM;
    }
    memcpy(link, symname, len);
    
    /* Luu link vao inode private data */
    inode->i_link = link;
    inode->i_size = len - 1;
    
    d_instantiate(dentry, inode);
    dget(dentry);
    
    return 0;
}

static const char *simplefs_get_link(struct dentry *dentry,
                                    struct inode *inode,
                                    struct delayed_call *done)
{
    return inode->i_link;
}

/* Cap nhat inode_operations */
static const struct inode_operations simplefs_symlink_inode_operations = {
    .get_link = simplefs_get_link,
};

/* Them vao dir_inode_operations */
static const struct inode_operations simplefs_dir_inode_operations = {
    .lookup = simplefs_lookup,
    .create = simplefs_create,
    .mkdir = simplefs_mkdir,
    .unlink = simplefs_unlink,
    .symlink = simplefs_symlink,  /* Them dong nay */
};
```

## 8. Thêm Rename Support

```c
static int simplefs_rename(struct inode *old_dir, struct dentry *old_dentry,
                          struct inode *new_dir, struct dentry *new_dentry,
                          unsigned int flags)
{
    struct inode *inode = d_inode(old_dentry);
    
    pr_info("simplefs: renaming '%s' to '%s'\n",
            old_dentry->d_name.name, new_dentry->d_name.name);
    
    /* Khong ho tro RENAME_EXCHANGE */
    if (flags & RENAME_EXCHANGE)
        return -EINVAL;
    
    /* Cap nhat timestamps */
    old_dir->i_ctime = old_dir->i_mtime = current_time(old_dir);
    new_dir->i_ctime = new_dir->i_mtime = current_time(new_dir);
    inode->i_ctime = current_time(inode);
    
    return 0;
}

/* Them vao dir_inode_operations */
static const struct inode_operations simplefs_dir_inode_operations = {
    .lookup = simplefs_lookup,
    .create = simplefs_create,
    .mkdir = simplefs_mkdir,
    .unlink = simplefs_unlink,
    .symlink = simplefs_symlink,
    .rename = simplefs_rename,  /* Them dong nay */
};
```

## 9. Persistent Storage với backing file

```c
/* Luu filesystem vao file tren disk */
static int simplefs_save_to_file(struct super_block *sb, const char *path)
{
    struct file *filp;
    loff_t pos = 0;
    ssize_t ret;
    char header[128];
    
    filp = filp_open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (IS_ERR(filp))
        return PTR_ERR(filp);
    
    /* Ghi header */
    snprintf(header, sizeof(header), "SIMPLEFS_V1\n");
    ret = kernel_write(filp, header, strlen(header), &pos);
    
    /* Ghi data cua cac file */
    /* ... implementation ... */
    
    filp_close(filp, NULL);
    return 0;
}

/* Doc filesystem tu file tren disk */
static int simplefs_load_from_file(struct super_block *sb, const char *path)
{
    struct file *filp;
    loff_t pos = 0;
    ssize_t ret;
    char header[128];
    
    filp = filp_open(path, O_RDONLY, 0);
    if (IS_ERR(filp))
        return PTR_ERR(filp);
    
    /* Doc header */
    ret = kernel_read(filp, header, sizeof(header) - 1, &pos);
    if (ret < 0) {
        filp_close(filp, NULL);
        return ret;
    }
    
    /* Verify header */
    if (strncmp(header, "SIMPLEFS_V1", 11) != 0) {
        filp_close(filp, NULL);
        return -EINVAL;
    }
    
    /* Doc data */
    /* ... implementation ... */
    
    filp_close(filp, NULL);
    return 0;
}
```

## 10. Thêm Permission Checks

```c
static int simplefs_permission(struct inode *inode, int mask)
{
    pr_info("simplefs: checking permission for inode %lu, mask 0x%x\n",
            inode->i_ino, mask);
    
    /* Su dung generic permission checker */
    return generic_permission(inode, mask);
}

/* Them vao inode_operations */
static const struct inode_operations simplefs_inode_operations = {
    /* ... cac operations khac ... */
    .permission = simplefs_permission,
};
```

## 11. Thêm Quota Support

```c
#include <linux/quotaops.h>

static int simplefs_write_with_quota(struct file *filp,
                                    const char __user *buf,
                                    size_t len, loff_t *ppos)
{
    struct inode *inode = file_inode(filp);
    int ret;
    
    /* Kiem tra quota */
    ret = dquot_alloc_space(inode, len);
    if (ret)
        return ret;
    
    /* Thuc hien write */
    ret = simplefs_write(filp, buf, len, ppos);
    
    if (ret < 0)
        dquot_free_space(inode, len);
    
    return ret;
}
```

## 12. Test các tính năng mới

### Test write support

```bash
sudo mount -t simplefs none /mnt/simplefs
echo "Hello World" > /mnt/simplefs/test.txt
cat /mnt/simplefs/test.txt
```

### Test create file

```bash
touch /mnt/simplefs/newfile.txt
ls -la /mnt/simplefs/
```

### Test mkdir

```bash
mkdir /mnt/simplefs/testdir
ls -la /mnt/simplefs/
```

### Test mount options

```bash
sudo mount -t simplefs -o mode=0777,uid=1000,gid=1000 none /mnt/simplefs
mount | grep simplefs
```

### Test symlink

```bash
ln -s /mnt/simplefs/hello /mnt/simplefs/link_to_hello
ls -la /mnt/simplefs/
cat /mnt/simplefs/link_to_hello
```

## Tips và Best Practices

1. **Memory Management**: Luôn free allocated memory trong error paths
2. **Locking**: Sử dụng appropriate locks để tránh race conditions
3. **Error Handling**: Return proper error codes
4. **Logging**: Sử dụng pr_debug() cho debug messages
5. **Testing**: Test thoroughly với different scenarios
6. **Reference Counting**: Đúng cách sử dụng iget/iput, dget/dput
7. **Namespace Aware**: Sử dụng current_user_ns() cho uid/gid conversions

## Debugging Advanced Features

### Trace filesystem operations

```bash
# Enable ftrace
echo 1 > /sys/kernel/debug/tracing/events/vfs/enable
echo 1 > /sys/kernel/debug/tracing/tracing_on

# Thuc hien operations
cat /mnt/simplefs/hello

# Xem trace
cat /sys/kernel/debug/tracing/trace
```

### Monitor inode cache

```bash
# Xem slab cache statistics
sudo cat /proc/slabinfo | grep -E "inode|dentry"
```

### Check memory leaks

```bash
# Truoc khi unload module
cat /proc/meminfo | grep -E "Slab|KReclaimable"

# Sau khi unload
# Compare values - neu tang la co memory leak
```

## Tài liệu tham khảo

- Linux source code: `fs/libfs.c` - Helper functions
- Linux source code: `fs/ramfs/` - Simple RAM filesystem
- Linux source code: `fs/tmpfs/` - Tmpfs implementation
- Linux Device Drivers Book, Chapter 18
- Understanding the Linux Kernel, Chapter 12
- Kernel documentation: `Documentation/filesystems/vfs.txt`

