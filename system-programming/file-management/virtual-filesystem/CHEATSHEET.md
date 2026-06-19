# SimplFS Kernel Module - Cheatsheet

## Các lệnh cơ bản

### Build và Load Module

```bash
# Build module
make

# Load module vao kernel
sudo insmod vfs_module.ko

# Kiem tra module da load
lsmod | grep vfs_module

# Xem thong tin module
modinfo vfs_module.ko

# Unload module
sudo rmmod vfs_module

# Xem kernel log
dmesg | tail -20
dmesg | grep simplefs
```

### Mount và Umount

```bash
# Tao mount point
sudo mkdir -p /mnt/simplefs

# Mount filesystem
sudo mount -t simplefs none /mnt/simplefs

# Kiem tra mount
mount | grep simplefs
findmnt /mnt/simplefs

# Xem noi dung
ls -la /mnt/simplefs
cat /mnt/simplefs/hello
cat /mnt/simplefs/info

# Stat filesystem
stat -f /mnt/simplefs
df -h /mnt/simplefs

# Umount
sudo umount /mnt/simplefs
```

### Test nhanh

```bash
# Chay tat ca test tu dong
sudo ./test_vfs.sh

# Hoac dung make
sudo make test

# Reload module
sudo make reload
```

## Cac Kernel VFS APIs quan trong

### 1. Filesystem Registration

```c
/* Dang ky filesystem type */
int register_filesystem(struct file_system_type *fs);

/* Huy dang ky filesystem type */
int unregister_filesystem(struct file_system_type *fs);

/* Dinh nghia filesystem type */
struct file_system_type {
    const char *name;           /* Ten filesystem */
    int fs_flags;               /* Flags */
    struct dentry *(*mount)(    /* Ham mount */
        struct file_system_type *,
        int, const char *, void *);
    void (*kill_sb)(            /* Ham umount */
        struct super_block *);
    struct module *owner;
};
```

### 2. Superblock Operations

```c
/* Cac thao tac voi superblock */
struct super_operations {
    struct inode *(*alloc_inode)(struct super_block *);
    void (*destroy_inode)(struct inode *);
    void (*put_super)(struct super_block *);
    int (*statfs)(struct dentry *, struct kstatfs *);
    int (*remount_fs)(struct super_block *, int *, char *);
    void (*clear_inode)(struct inode *);
    int (*drop_inode)(struct inode *);
};

/* Fill superblock */
int fill_super(struct super_block *sb, void *data, int silent);
```

### 3. Inode Operations

```c
/* Tao inode moi */
struct inode *new_inode(struct super_block *sb);

/* Khoi tao owner cua inode */
void inode_init_owner(struct user_namespace *ns,
                     struct inode *inode,
                     const struct inode *dir,
                     umode_t mode);

/* Lay ino number tiep theo */
unsigned int get_next_ino(void);

/* Directory inode operations */
struct inode_operations {
    struct dentry *(*lookup)(struct inode *, 
                            struct dentry *, 
                            unsigned int);
    int (*create)(struct inode *, struct dentry *, 
                 umode_t, bool);
    int (*mkdir)(struct inode *, struct dentry *, 
                umode_t);
    int (*rmdir)(struct inode *, struct dentry *);
    int (*rename)(struct inode *, struct dentry *,
                 struct inode *, struct dentry *,
                 unsigned int);
};
```

### 4. File Operations

```c
/* File operations cho regular files */
struct file_operations {
    ssize_t (*read)(struct file *, char __user *, 
                   size_t, loff_t *);
    ssize_t (*write)(struct file *, const char __user *,
                    size_t, loff_t *);
    int (*open)(struct inode *, struct file *);
    int (*release)(struct inode *, struct file *);
    loff_t (*llseek)(struct file *, loff_t, int);
};

/* Directory operations */
struct file_operations {
    int (*iterate_shared)(struct file *, 
                         struct dir_context *);
    ssize_t (*read)(struct file *, char __user *,
                   size_t, loff_t *);
};
```

### 5. Dentry Operations

```c
/* Tao root dentry */
struct dentry *d_make_root(struct inode *root_inode);

/* Them inode vao dentry */
void d_add(struct dentry *entry, struct inode *inode);

/* Emit dot entries (. va ..) */
bool dir_emit_dots(struct file *file, struct dir_context *ctx);

/* Emit directory entry */
bool dir_emit(struct dir_context *ctx, const char *name,
             int namelen, u64 ino, unsigned type);
```

### 6. Mount Operations

```c
/* Mount filesystem khong can device */
struct dentry *mount_nodev(struct file_system_type *fs_type,
                          int flags, void *data,
                          int (*fill_super)(struct super_block *,
                                          void *, int));

/* Kill superblock */
void kill_litter_super(struct super_block *sb);
void kill_anon_super(struct super_block *sb);
```

### 7. User Space Copy

```c
/* Copy du lieu tu kernel sang user space */
unsigned long copy_to_user(void __user *to,
                          const void *from,
                          unsigned long n);

/* Copy du lieu tu user space vao kernel */
unsigned long copy_from_user(void *to,
                            const void __user *from,
                            unsigned long n);
```

### 8. Helper Functions

```c
/* Lay inode tu file structure */
struct inode *file_inode(const struct file *file);

/* Lay current time */
struct timespec64 current_time(const struct inode *inode);

/* Generic file operations */
loff_t generic_file_llseek(struct file *file, loff_t offset,
                          int whence);
ssize_t generic_read_dir(struct file *filp, char __user *buf,
                        size_t siz, loff_t *ppos);
```

## Cac thanh phan chinh cua VFS

### 1. Superblock

Dai dien cho mot filesystem instance da duoc mount

```c
struct super_block {
    unsigned long s_magic;      /* Magic number */
    struct super_operations *s_op;  /* Operations */
    struct dentry *s_root;      /* Root dentry */
    unsigned long s_blocksize;  /* Block size */
    unsigned char s_blocksize_bits;
    loff_t s_maxbytes;         /* Max file size */
    unsigned long s_time_gran;  /* Time granularity */
};
```

### 2. Inode

Dai dien cho mot file hoac directory trong filesystem

```c
struct inode {
    umode_t i_mode;            /* Permissions */
    uid_t i_uid;               /* Owner */
    gid_t i_gid;               /* Group */
    unsigned long i_ino;       /* Inode number */
    struct timespec64 i_atime; /* Access time */
    struct timespec64 i_mtime; /* Modification time */
    struct timespec64 i_ctime; /* Change time */
    const struct inode_operations *i_op;
    const struct file_operations *i_fop;
};
```

### 3. Dentry (Directory Entry)

Ket noi ten file voi inode tuong ung

```c
struct dentry {
    struct inode *d_inode;     /* Inode */
    struct dentry *d_parent;   /* Parent dentry */
    struct qstr d_name;        /* File name */
    struct super_block *d_sb;  /* Superblock */
};
```

### 4. File

Dai dien cho mot file dang mo

```c
struct file {
    struct path f_path;        /* Path */
    struct inode *f_inode;     /* Inode */
    const struct file_operations *f_op;
    loff_t f_pos;             /* File position */
    unsigned int f_flags;      /* Flags */
    fmode_t f_mode;           /* Mode */
};
```

## Cac loai Inode Mode

```c
S_IFMT    0170000  /* Bit mask cho file type */
S_IFSOCK  0140000  /* Socket */
S_IFLNK   0120000  /* Symbolic link */
S_IFREG   0100000  /* Regular file */
S_IFBLK   0060000  /* Block device */
S_IFDIR   0040000  /* Directory */
S_IFCHR   0020000  /* Character device */
S_IFIFO   0010000  /* FIFO */

S_ISUID   0004000  /* Set UID bit */
S_ISGID   0002000  /* Set GID bit */
S_ISVTX   0001000  /* Sticky bit */

S_IRWXU   00700    /* User rwx */
S_IRUSR   00400    /* User read */
S_IWUSR   00200    /* User write */
S_IXUSR   00100    /* User execute */

S_IRWXG   00070    /* Group rwx */
S_IRWXO   00007    /* Other rwx */
```

## Cac Directory Entry Type

```c
DT_UNKNOWN  0   /* Unknown type */
DT_FIFO     1   /* Named pipe (FIFO) */
DT_CHR      2   /* Character device */
DT_DIR      4   /* Directory */
DT_BLK      6   /* Block device */
DT_REG      8   /* Regular file */
DT_LNK      10  /* Symbolic link */
DT_SOCK     12  /* UNIX domain socket */
DT_WHT      14  /* Whiteout */
```

## Debugging

### Kernel Log Levels

```c
pr_emerg()   /* Emergency - system khong su dung duoc */
pr_alert()   /* Alert - can xu ly ngay lap tuc */
pr_crit()    /* Critical */
pr_err()     /* Error */
pr_warn()    /* Warning */
pr_notice()  /* Notice - binh thuong nhung quan trong */
pr_info()    /* Informational */
pr_debug()   /* Debug - chi hien thi khi DEBUG duoc bat */
```

### Xem Kernel Log

```bash
# Xem tat ca kernel log
dmesg

# Xem 20 dong cuoi
dmesg | tail -20

# Loc theo module
dmesg | grep simplefs

# Theo doi real-time
dmesg -w

# Xoa log
sudo dmesg -c
```

### Debug Tips

1. **Them log messages:** Su dung pr_info() de trace execution
2. **Kiem tra return values:** Luon kiem tra return value cua ham
3. **Xem /proc/filesystems:** Kiem tra filesystem da dang ky
4. **Xem /proc/mounts:** Kiem tra mount points
5. **Su dung printk_ratelimited():** Tranh spam log

## Loi thuong gap

### 1. Module khong load duoc

```bash
# Kiem tra kernel version
uname -r

# Kiem tra kernel headers
ls /lib/modules/$(uname -r)/build

# Cai kernel headers
sudo apt-get install linux-headers-$(uname -r)  # Ubuntu/Debian
sudo dnf install kernel-devel kernel-headers     # Fedora/RHEL
```

### 2. Mount that bai

```bash
# Kiem tra module da load
lsmod | grep vfs_module

# Kiem tra filesystem type
cat /proc/filesystems | grep simplefs

# Xem loi chi tiet
dmesg | tail -20

# Thu mount voi debug
sudo mount -t simplefs -v none /mnt/simplefs
```

### 3. Khong umount duoc

```bash
# Kiem tra process nao dang su dung
lsof +D /mnt/simplefs
fuser -m /mnt/simplefs

# Force umount
sudo umount -f /mnt/simplefs

# Lazy umount
sudo umount -l /mnt/simplefs
```

### 4. Module khong unload duoc

```bash
# Kiem tra module co dang su dung
lsmod | grep vfs_module

# Phai umount truoc
sudo umount /mnt/simplefs

# Roi moi unload
sudo rmmod vfs_module
```

## Tai lieu tham khao

- Linux Kernel Documentation: Documentation/filesystems/
- `/usr/src/linux/fs/` - Source code cac filesystem
- `man 2 mount` - Mount system call
- `man 2 statfs` - Statfs system call
- Linux Device Drivers, 3rd Edition
- Understanding the Linux Kernel, 3rd Edition

## Vi du mo rong

### Them file moi

Sua trong `simplefs_iterate()` va `simplefs_lookup()`:

```c
/* Them file 'test' */
if (ctx->pos == 4) {
    if (!dir_emit(ctx, "test", 4, SIMPLEFS_TEST_INO, DT_REG))
        return 0;
    ctx->pos++;
}
```

### Them write support

Implement ham write trong file_operations:

```c
static ssize_t simplefs_write(struct file *filp, 
                             const char __user *buf,
                             size_t len, loff_t *ppos)
{
    /* Implementation */
}
```

### Them directory support

Implement mkdir trong inode_operations:

```c
static int simplefs_mkdir(struct inode *dir, 
                         struct dentry *dentry,
                         umode_t mode)
{
    /* Implementation */
}
```

