# File Management - Quản lý File trong Linux

## Tổng quan

Thư mục này chứa các ví dụ về quản lý file (file management) trong Linux, bao gồm các API và kỹ thuật quan trọng để làm việc với file system, từ các thao tác cơ bản đến nâng cao như file locking, memory mapping, và inotify.

## Cấu trúc Project

```
file/
├── file_operations.c    - Các thao tác file cơ bản
├── file_locking.c       - File locking và synchronization
├── memory_map.c         - Memory-mapped files (mmap)
├── directory_walk.c     - Duyệt cây thư mục
├── inotify_example.c    - Giám sát thay đổi file/directory
├── vfs_module/          - VFS kernel module
└── Makefile            - Build system
```

## Build và Run

### Build tất cả chương trình
```bash
make all
```

### Build từng chương trình riêng lẻ
```bash
make file_operations
make file_locking
make memory_map
make directory_walk
make inotify_example
```

### Dọn dẹp
```bash
make clean
```

## Kiến thức cơ bản

### 1. File Descriptor

File descriptor (FD) là số nguyên dùng để tham chiếu đến file đã mở:
- **0**: stdin (standard input)
- **1**: stdout (standard output)
- **2**: stderr (standard error)
- **3+**: File do người dùng mở

### 2. File I/O Models

Linux hỗ trợ 2 loại API chính:

**System calls (unbuffered I/O):**
- `open()`, `read()`, `write()`, `close()`
- Gọi trực tiếp kernel
- Hiệu suất cao cho I/O lớn

**Standard I/O (buffered I/O):**
- `fopen()`, `fread()`, `fwrite()`, `fclose()`
- Có buffer trong user space
- Tiện lợi cho I/O nhỏ

### 3. Virtual File System (VFS)

VFS là lớp trừu tượng trong kernel cho phép:
- Hỗ trợ nhiều file systems (ext4, xfs, nfs, ...)
- API thống nhất cho user space
- Các khái niệm: inode, dentry, superblock

---

## Chi tiết từng chương trình

## 1. file_operations.c - Thao tác file cơ bản

### Kiến thức sử dụng

#### API: `open()`

```c
#include <fcntl.h>

int open(const char *pathname, int flags);
int open(const char *pathname, int flags, mode_t mode);
```

**Flags:**
- `O_RDONLY`: Chỉ đọc
- `O_WRONLY`: Chỉ ghi
- `O_RDWR`: Đọc và ghi
- `O_CREAT`: Tạo file nếu chưa tồn tại
- `O_TRUNC`: Xóa nội dung file
- `O_APPEND`: Ghi vào cuối file
- `O_EXCL`: Lỗi nếu file đã tồn tại (dùng với O_CREAT)
- `O_NONBLOCK`: Non-blocking I/O
- `O_SYNC`: Synchronous writes

**Mode (permissions):**
```c
S_IRUSR (0400)  - User read
S_IWUSR (0200)  - User write
S_IXUSR (0100)  - User execute
S_IRGRP (0040)  - Group read
S_IWGRP (0020)  - Group write
S_IXGRP (0010)  - Group execute
S_IROTH (0004)  - Others read
S_IWOTH (0002)  - Others write
S_IXOTH (0001)  - Others execute
```

#### API: `read()`, `write()`

```c
#include <unistd.h>

ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
```

**Return value:**
- Số bytes đã đọc/ghi
- 0 = EOF (end of file) cho read()
- -1 = lỗi

#### API: `lseek()`

Di chuyển file offset (con trỏ file):

```c
#include <unistd.h>

off_t lseek(int fd, off_t offset, int whence);
```

**whence:**
- `SEEK_SET`: Từ đầu file
- `SEEK_CUR`: Từ vị trí hiện tại
- `SEEK_END`: Từ cuối file

#### API: `close()`

```c
#include <unistd.h>

int close(int fd);
```

Đóng file descriptor, giải phóng tài nguyên.

#### API: `stat()`, `fstat()`, `lstat()`

Lấy thông tin file:

```c
#include <sys/stat.h>

int stat(const char *pathname, struct stat *statbuf);
int fstat(int fd, struct stat *statbuf);
int lstat(const char *pathname, struct stat *statbuf);

struct stat {
    dev_t     st_dev;      // Device ID
    ino_t     st_ino;      // Inode number
    mode_t    st_mode;     // File type và permissions
    nlink_t   st_nlink;    // Số hard links
    uid_t     st_uid;      // User ID
    gid_t     st_gid;      // Group ID
    dev_t     st_rdev;     // Device ID (nếu là special file)
    off_t     st_size;     // Kích thước file (bytes)
    blksize_t st_blksize;  // Block size cho I/O
    blkcnt_t  st_blocks;   // Số blocks được cấp phát
    time_t    st_atime;    // Thời gian access cuối
    time_t    st_mtime;    // Thời gian modify cuối
    time_t    st_ctime;    // Thời gian change status cuối
};
```

**Macros kiểm tra file type:**
- `S_ISREG(m)`: File thông thường?
- `S_ISDIR(m)`: Directory?
- `S_ISLNK(m)`: Symbolic link?
- `S_ISBLK(m)`: Block device?
- `S_ISCHR(m)`: Character device?
- `S_ISFIFO(m)`: FIFO (pipe)?
- `S_ISSOCK(m)`: Socket?

#### Ví dụ đọc/ghi file

```c
int fd = open("test.txt", O_RDWR | O_CREAT, 0644);
if (fd < 0) {
    perror("open");
    exit(1);
}

char buf[1024];
ssize_t n = read(fd, buf, sizeof(buf));

write(fd, "Hello\n", 6);

close(fd);
```

---

## 2. file_locking.c - File Locking

### Kiến thức sử dụng

#### Tại sao cần file locking?

Khi nhiều processes truy cập cùng một file:
- Đọc đồng thời: OK
- Ghi đồng thời: Dữ liệu bị hỏng
- Đọc + Ghi đồng thời: Đọc dữ liệu không nhất quán

File locking đảm bảo đồng bộ hóa.

#### Loại locks

**Advisory locks (khuyến nghị):**
- Processes tự nguyện tuân thủ
- Không enforce bởi kernel
- `fcntl()`, `flock()`

**Mandatory locks (bắt buộc):**
- Kernel enforce
- Hiếm dùng, có thể gây deadlock
- Cần mount option đặc biệt

#### API: `fcntl()` - POSIX record locking

```c
#include <fcntl.h>

int fcntl(int fd, int cmd, struct flock *lock);

struct flock {
    short l_type;    // F_RDLCK, F_WRLCK, F_UNLCK
    short l_whence;  // SEEK_SET, SEEK_CUR, SEEK_END
    off_t l_start;   // Offset bắt đầu
    off_t l_len;     // Độ dài (0 = đến EOF)
    pid_t l_pid;     // PID (F_GETLK)
};
```

**Commands:**
- `F_SETLK`: Đặt lock (non-blocking)
- `F_SETLKW`: Đặt lock (blocking, wait)
- `F_GETLK`: Kiểm tra lock

**Lock types:**
- `F_RDLCK`: Read lock (shared)
- `F_WRLCK`: Write lock (exclusive)
- `F_UNLCK`: Unlock

#### API: `flock()` - BSD file locking

```c
#include <sys/file.h>

int flock(int fd, int operation);
```

**Operations:**
- `LOCK_SH`: Shared lock (đọc)
- `LOCK_EX`: Exclusive lock (ghi)
- `LOCK_UN`: Unlock
- `LOCK_NB`: Non-blocking (OR với LOCK_SH/LOCK_EX)

#### Ví dụ fcntl locking

```c
int fd = open("data.txt", O_RDWR);

struct flock lock;
lock.l_type = F_WRLCK;
lock.l_whence = SEEK_SET;
lock.l_start = 0;
lock.l_len = 0;

if (fcntl(fd, F_SETLKW, &lock) == -1) {
    perror("fcntl");
}

write(fd, "data", 4);

lock.l_type = F_UNLCK;
fcntl(fd, F_SETLK, &lock);

close(fd);
```

#### So sánh fcntl vs flock

| Tính năng | fcntl() | flock() |
|-----------|---------|---------|
| Standard | POSIX | BSD |
| Phạm vi | Byte range | Toàn file |
| Kế thừa | Không qua fork | Qua fork |
| Network | Hỗ trợ NFS | Không |

---

## 3. memory_map.c - Memory-Mapped Files

### Kiến thức sử dụng

#### Memory mapping là gì?

Map file vào address space của process:
- File được coi như một mảng trong memory
- Truy cập qua con trỏ thay vì read()/write()
- Kernel tự động sync giữa memory và disk

#### API: `mmap()`

```c
#include <sys/mman.h>

void *mmap(void *addr, size_t length, int prot, int flags,
           int fd, off_t offset);
```

**Parameters:**
- `addr`: Địa chỉ gợi ý (thường dùng NULL để kernel chọn)
- `length`: Kích thước vùng map (bytes)
- `prot`: Protection flags
- `flags`: Mapping flags
- `fd`: File descriptor
- `offset`: Offset trong file (phải là bội số của page size)

**Protection flags (prot):**
- `PROT_READ`: Có thể đọc
- `PROT_WRITE`: Có thể ghi
- `PROT_EXEC`: Có thể thực thi
- `PROT_NONE`: Không truy cập được

**Mapping flags:**
- `MAP_SHARED`: Thay đổi được share với processes khác và ghi vào file
- `MAP_PRIVATE`: Copy-on-write, thay đổi không ảnh hưởng file gốc
- `MAP_ANONYMOUS`: Map memory không gắn với file
- `MAP_FIXED`: Yêu cầu địa chỉ chính xác
- `MAP_POPULATE`: Prefault page tables
- `MAP_LOCKED`: Lock pages vào RAM

**Return value:**
- Địa chỉ vùng mapped thành công
- `MAP_FAILED` ((void *) -1) nếu lỗi

#### API: `munmap()`

```c
int munmap(void *addr, size_t length);
```

Hủy mapping, giải phóng virtual memory.

#### API: `msync()`

```c
int msync(void *addr, size_t length, int flags);
```

Đồng bộ memory với file trên disk.

**Flags:**
- `MS_SYNC`: Đồng bộ đồng bộ (blocking)
- `MS_ASYNC`: Đồng bộ bất đồng bộ (non-blocking)
- `MS_INVALIDATE`: Invalidate cached copies

#### API: `mprotect()`

```c
int mprotect(void *addr, size_t len, int prot);
```

Thay đổi protection của vùng memory đã map.

#### Ví dụ sử dụng mmap

```c
int fd = open("data.bin", O_RDWR);
struct stat sb;
fstat(fd, &sb);

char *addr = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE,
                  MAP_SHARED, fd, 0);
if (addr == MAP_FAILED) {
    perror("mmap");
    exit(1);
}

addr[0] = 'A';

msync(addr, sb.st_size, MS_SYNC);

munmap(addr, sb.st_size);
close(fd);
```

#### Ưu điểm của mmap

- Hiệu suất cao cho truy cập ngẫu nhiên
- Không cần buffer trong user space
- Nhiều processes có thể share memory
- Kernel tự động cache management
- Tận dụng virtual memory và page cache

#### Nhược điểm của mmap

- Giới hạn bởi address space (32-bit: 4GB)
- Overhead cho file nhỏ
- Phức tạp hơn read()/write()
- Cần xử lý SIGBUS khi file bị truncate

---

## 4. directory_walk.c - Duyệt cây thư mục

### Kiến thức sử dụng

#### API: `opendir()`, `readdir()`, `closedir()`

```c
#include <dirent.h>

DIR *opendir(const char *name);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);
```

**struct dirent:**
```c
struct dirent {
    ino_t          d_ino;       // Inode number
    off_t          d_off;       // Offset đến dirent tiếp theo
    unsigned short d_reclen;    // Độ dài của record
    unsigned char  d_type;      // File type
    char           d_name[256]; // Tên file
};
```

**d_type values:**
- `DT_REG`: File thông thường
- `DT_DIR`: Directory
- `DT_LNK`: Symbolic link
- `DT_FIFO`: Named pipe
- `DT_SOCK`: Socket
- `DT_CHR`: Character device
- `DT_BLK`: Block device
- `DT_UNKNOWN`: Unknown type

#### API: `scandir()`

```c
#include <dirent.h>

int scandir(const char *dirp, struct dirent ***namelist,
            int (*filter)(const struct dirent *),
            int (*compar)(const struct dirent **, const struct dirent **));
```

Quét directory và trả về danh sách entries đã sắp xếp.

**Filter function:**
- Return 1: Giữ entry
- Return 0: Loại bỏ entry

**Compare function:**
- `alphasort`: Sắp xếp theo tên
- `versionsort`: Sắp xếp theo version
- Custom function

#### API: `nftw()` - File tree walk

```c
#include <ftw.h>

int nftw(const char *dirpath,
         int (*fn)(const char *fpath, const struct stat *sb,
                   int typeflag, struct FTW *ftwbuf),
         int nopenfd, int flags);
```

Duyệt cây thư mục một cách đệ quy.

**Flags:**
- `FTW_PHYS`: Không follow symbolic links
- `FTW_MOUNT`: Không cross mount points
- `FTW_DEPTH`: Post-order traversal (children trước parent)
- `FTW_CHDIR`: Chdir vào mỗi directory

**typeflag values:**
- `FTW_F`: File thông thường
- `FTW_D`: Directory
- `FTW_DNR`: Directory không đọc được
- `FTW_SL`: Symbolic link
- `FTW_NS`: stat() thất bại

#### Ví dụ duyệt directory

```c
DIR *dir = opendir("/tmp");
if (dir == NULL) {
    perror("opendir");
    exit(1);
}

struct dirent *entry;
while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || 
        strcmp(entry->d_name, "..") == 0) {
        continue;
    }
    printf("%s (type=%d)\n", entry->d_name, entry->d_type);
}

closedir(dir);
```

#### Ví dụ duyệt đệ quy với nftw

```c
int callback(const char *fpath, const struct stat *sb,
             int typeflag, struct FTW *ftwbuf)
{
    printf("%s\n", fpath + ftwbuf->base);
    return 0;
}

nftw("/home/user", callback, 20, FTW_PHYS);
```

---

## 5. inotify_example.c - Giám sát thay đổi file

### Kiến thức sử dụng

#### Inotify là gì?

Inotify là API để giám sát các sự kiện file system:
- Tạo/xóa file
- Sửa đổi nội dung
- Thay đổi metadata
- Di chuyển/đổi tên file

#### API: `inotify_init()`, `inotify_init1()`

```c
#include <sys/inotify.h>

int inotify_init(void);
int inotify_init1(int flags);
```

Tạo inotify instance, trả về file descriptor.

**Flags:**
- `IN_NONBLOCK`: Non-blocking reads
- `IN_CLOEXEC`: Close-on-exec

#### API: `inotify_add_watch()`

```c
int inotify_add_watch(int fd, const char *pathname, uint32_t mask);
```

Thêm watch cho một file/directory.

**Watch mask (events):**
- `IN_ACCESS`: File được đọc
- `IN_MODIFY`: File được ghi
- `IN_ATTRIB`: Metadata thay đổi
- `IN_CLOSE_WRITE`: File đóng sau khi ghi
- `IN_CLOSE_NOWRITE`: File đóng không có ghi
- `IN_OPEN`: File được mở
- `IN_MOVED_FROM`: File được di chuyển đi
- `IN_MOVED_TO`: File được di chuyển đến
- `IN_CREATE`: File/directory được tạo
- `IN_DELETE`: File/directory bị xóa
- `IN_DELETE_SELF`: Watched file/directory bị xóa

**Convenience masks:**
- `IN_CLOSE`: (IN_CLOSE_WRITE | IN_CLOSE_NOWRITE)
- `IN_MOVE`: (IN_MOVED_FROM | IN_MOVED_TO)
- `IN_ALL_EVENTS`: Tất cả events

**Additional flags:**
- `IN_ONLYDIR`: Chỉ watch nếu là directory
- `IN_DONT_FOLLOW`: Không follow symbolic links
- `IN_EXCL_UNLINK`: Không report events khi file unlinked
- `IN_MASK_ADD`: Add events vào existing watch
- `IN_ONESHOT`: Giám sát một lần rồi tự động remove

#### API: `inotify_rm_watch()`

```c
int inotify_rm_watch(int fd, int wd);
```

Xóa watch.

#### Đọc events

```c
struct inotify_event {
    int      wd;       // Watch descriptor
    uint32_t mask;     // Event mask
    uint32_t cookie;   // Cookie để link events (rename)
    uint32_t len;      // Độ dài của name
    char     name[];   // Tên file (optional)
};
```

Events được đọc từ inotify fd bằng `read()`. Mỗi read có thể trả về nhiều events.

#### Ví dụ sử dụng inotify

```c
int fd = inotify_init1(IN_NONBLOCK);
if (fd == -1) {
    perror("inotify_init1");
    exit(1);
}

int wd = inotify_add_watch(fd, "/tmp", 
                           IN_CREATE | IN_DELETE | IN_MODIFY);
if (wd == -1) {
    perror("inotify_add_watch");
    exit(1);
}

char buf[4096] __attribute__((aligned(__alignof__(struct inotify_event))));

while (1) {
    ssize_t len = read(fd, buf, sizeof(buf));
    if (len == -1 && errno != EAGAIN) {
        perror("read");
        break;
    }
    
    if (len <= 0)
        break;
    
    char *ptr = buf;
    while (ptr < buf + len) {
        struct inotify_event *event = (struct inotify_event *)ptr;
        
        if (event->mask & IN_CREATE)
            printf("Created: %s\n", event->name);
        if (event->mask & IN_DELETE)
            printf("Deleted: %s\n", event->name);
        if (event->mask & IN_MODIFY)
            printf("Modified: %s\n", event->name);
        
        ptr += sizeof(struct inotify_event) + event->len;
    }
}

inotify_rm_watch(fd, wd);
close(fd);
```

#### Lưu ý khi dùng inotify

- Inotify không theo dõi đệ quy subdirectories (phải add watch riêng)
- Events có thể bị merge nếu xảy ra quá nhanh
- Buffer đầy có thể dẫn đến mất events (IN_Q_OVERFLOW)
- Không hoạt động với network file systems
- Giới hạn số watches (check /proc/sys/fs/inotify/max_user_watches)

---

## 6. VFS Module - Virtual File System Kernel Module

### Kiến thức sử dụng

#### VFS trong Linux Kernel

VFS (Virtual File System) là lớp trừu tượng trong kernel:
- Cung cấp interface thống nhất cho nhiều file systems
- Các thành phần chính: superblock, inode, dentry, file

#### Cấu trúc dữ liệu quan trọng

**Superblock:**
```c
struct super_block {
    struct list_head    s_list;
    dev_t               s_dev;
    unsigned long       s_blocksize;
    loff_t              s_maxbytes;
    struct file_system_type *s_type;
    struct super_operations *s_op;
    struct dentry       *s_root;
    ...
};
```

Chứa thông tin về file system đã mount.

**Inode:**
```c
struct inode {
    umode_t             i_mode;
    uid_t               i_uid;
    gid_t               i_gid;
    loff_t              i_size;
    struct timespec     i_atime;
    struct timespec     i_mtime;
    struct timespec     i_ctime;
    struct inode_operations *i_op;
    struct file_operations  *i_fop;
    struct super_block  *i_sb;
    ...
};
```

Đại diện cho một file trên disk (metadata).

**Dentry:**
```c
struct dentry {
    struct dentry       *d_parent;
    struct qstr         d_name;
    struct inode        *d_inode;
    struct dentry_operations *d_op;
    struct super_block  *d_sb;
    ...
};
```

Directory entry, link giữa tên file và inode.

**File:**
```c
struct file {
    struct path         f_path;
    struct inode        *f_inode;
    struct file_operations *f_op;
    loff_t              f_pos;
    unsigned int        f_flags;
    fmode_t             f_mode;
    ...
};
```

Đại diện cho file đã mở trong process.

#### Operations structures

**file_operations:**
```c
struct file_operations {
    struct module *owner;
    loff_t (*llseek) (struct file *, loff_t, int);
    ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
    ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
    int (*open) (struct inode *, struct file *);
    int (*release) (struct inode *, struct file *);
    int (*fsync) (struct file *, loff_t, loff_t, int datasync);
    ...
};
```

**inode_operations:**
```c
struct inode_operations {
    struct dentry * (*lookup) (struct inode *, struct dentry *, unsigned int);
    int (*create) (struct inode *, struct dentry *, umode_t, bool);
    int (*link) (struct dentry *, struct inode *, struct dentry *);
    int (*unlink) (struct inode *, struct dentry *);
    int (*mkdir) (struct inode *, struct dentry *, umode_t);
    int (*rmdir) (struct inode *, struct dentry *);
    ...
};
```

**super_operations:**
```c
struct super_operations {
    struct inode *(*alloc_inode)(struct super_block *sb);
    void (*destroy_inode)(struct inode *);
    void (*dirty_inode) (struct inode *, int flags);
    int (*write_inode) (struct inode *, struct writeback_control *wbc);
    void (*put_super) (struct super_block *);
    int (*statfs) (struct dentry *, struct kstatfs *);
    ...
};
```

#### Viết simple file system module

**Các bước:**
1. Định nghĩa file_system_type
2. Implement mount function
3. Implement superblock operations
4. Implement inode operations
5. Implement file operations
6. Register/unregister file system

**Ví dụ skeleton:**
```c
#include <linux/fs.h>
#include <linux/module.h>

static struct dentry *myfs_mount(struct file_system_type *fs_type,
                                  int flags, const char *dev_name,
                                  void *data)
{
    return mount_nodev(fs_type, flags, data, myfs_fill_super);
}

static struct file_system_type myfs_type = {
    .owner = THIS_MODULE,
    .name = "myfs",
    .mount = myfs_mount,
    .kill_sb = kill_litter_super,
};

static int __init myfs_init(void)
{
    return register_filesystem(&myfs_type);
}

static void __exit myfs_exit(void)
{
    unregister_filesystem(&myfs_type);
}

module_init(myfs_init);
module_exit(myfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Simple VFS example");
```

---

## Kiến thức nâng cao

### 1. I/O Models và Performance

#### Buffered I/O vs Direct I/O

**Buffered I/O (mặc định):**
- Data đi qua page cache
- Kernel tự động cache và prefetch
- Tốt cho hầu hết use cases

**Direct I/O (O_DIRECT):**
- Bỏ qua page cache
- Truyền trực tiếp giữa user buffer và disk
- Yêu cầu alignment (buffer và offset)
- Tốt cho databases và applications tự quản lý cache

```c
int fd = open("file.dat", O_RDWR | O_DIRECT);
```

#### Synchronous vs Asynchronous I/O

**Synchronous:**
- `read()`, `write()` block cho đến hoàn thành
- Đơn giản nhưng không hiệu quả cho nhiều operations

**Asynchronous (AIO):**
- Submit operations và tiếp tục làm việc khác
- Nhận notification khi hoàn thành
- APIs: POSIX AIO (`aio_read`, `aio_write`), Linux native AIO (`io_submit`)

#### I/O Scheduling

Linux có nhiều I/O schedulers:
- **noop**: FIFO, cho SSD
- **deadline**: Đảm bảo latency
- **cfq**: Completely Fair Queuing
- **bfq**: Budget Fair Queueing

Check/thay đổi:
```bash
cat /sys/block/sda/queue/scheduler
echo deadline > /sys/block/sda/queue/scheduler
```

### 2. File System Features

#### Extended Attributes (xattr)

Metadata bổ sung cho files:

```c
#include <sys/xattr.h>

int setxattr(const char *path, const char *name,
             const void *value, size_t size, int flags);
int getxattr(const char *path, const char *name,
             void *value, size_t size);
int listxattr(const char *path, char *list, size_t size);
int removexattr(const char *path, const char *name);
```

**Namespaces:**
- `user.*`: User-defined attributes
- `trusted.*`: Privileged attributes
- `system.*`: System attributes (ACLs)
- `security.*`: Security modules (SELinux)

#### Access Control Lists (ACL)

Permissions mở rộng hơn user/group/other:

```bash
getfacl file.txt
setfacl -m u:john:rw file.txt
```

#### Sparse Files

Files với holes (vùng không ghi data):

```c
int fd = open("sparse.dat", O_WRONLY | O_CREAT, 0644);
lseek(fd, 1024*1024*1024, SEEK_SET);  // 1GB
write(fd, "end", 3);
close(fd);
```

File 1GB nhưng chỉ chiếm vài KB trên disk.

#### File Capabilities

Gán privileges cho executables không cần setuid:

```bash
getcap /bin/ping
setcap cap_net_raw+ep /bin/ping
```

### 3. Performance Optimization

#### Prefetching và Readahead

```c
#include <fcntl.h>

posix_fadvise(fd, offset, len, POSIX_FADV_WILLNEED);  // Prefetch
posix_fadvise(fd, offset, len, POSIX_FADV_SEQUENTIAL); // Sequential hint
```

#### Memory-mapped I/O best practices

- Dùng `MAP_POPULATE` để prefault pages
- Dùng `madvise()` để hint access patterns:
  ```c
  madvise(addr, length, MADV_SEQUENTIAL);
  madvise(addr, length, MADV_RANDOM);
  madvise(addr, length, MADV_WILLNEED);
  ```

#### Batch operations

Giảm system calls bằng cách batch:
- `readv()`, `writev()`: Vectored I/O
- `pread()`, `pwrite()`: Positioned I/O (không thay đổi offset)

```c
struct iovec iov[3];
iov[0].iov_base = buf1;
iov[0].iov_len = len1;
iov[1].iov_base = buf2;
iov[1].iov_len = len2;
iov[2].iov_base = buf3;
iov[2].iov_len = len3;

writev(fd, iov, 3);
```

---

## Best Practices

### 1. Error Handling

Luôn kiểm tra return values:

```c
int fd = open("file.txt", O_RDONLY);
if (fd == -1) {
    perror("open");
    // Handle error
    exit(1);
}

ssize_t n = read(fd, buf, sizeof(buf));
if (n == -1) {
    perror("read");
    close(fd);
    exit(1);
}

if (close(fd) == -1) {
    perror("close");
}
```

### 2. Resource Management

**Luôn đóng file descriptors:**
```c
int fd = open(...);
if (fd == -1) return -1;

// Do work
int ret = do_something(fd);

// Cleanup
close(fd);
return ret;
```

**Giới hạn số FDs:**
```bash
ulimit -n        # Check limit
ulimit -n 4096   # Set limit
```

### 3. Security

**Tránh race conditions:**
```c
// BAD: TOCTOU (Time-of-check-time-of-use)
if (access("file", W_OK) == 0) {
    fd = open("file", O_WRONLY);  // File có thể bị thay đổi ở đây
}

// GOOD: Mở trực tiếp và check error
fd = open("file", O_WRONLY);
if (fd == -1) {
    if (errno == EACCES) {
        // No permission
    }
}
```

**Sử dụng O_NOFOLLOW:**
```c
// Tránh symbolic link attacks
int fd = open(path, O_RDONLY | O_NOFOLLOW);
```

**Secure file creation:**
```c
// Đảm bảo file mới, không overwrite existing file
int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
```

### 4. Portability

**Check feature availability:**
```c
#ifdef _POSIX_MAPPED_FILES
// mmap() available
#endif

#ifdef _POSIX_SYNCHRONIZED_IO
// fsync() available
#endif
```

**Handle EINTR:**
```c
ssize_t safe_read(int fd, void *buf, size_t count) {
    ssize_t n;
    do {
        n = read(fd, buf, count);
    } while (n == -1 && errno == EINTR);
    return n;
}
```

---

## Debugging và Troubleshooting

### 1. Công cụ debug

**strace - Trace system calls:**
```bash
strace ./program
strace -e open,read,write ./program
strace -p <pid>  # Attach vào running process
```

**lsof - List open files:**
```bash
lsof -p <pid>           # Files opened by process
lsof /path/to/file      # Processes using file
lsof -u username        # Files opened by user
```

**fuser - Identify processes using files:**
```bash
fuser -v /path/to/file
fuser -k /path/to/file  # Kill processes
```

### 2. Kiểm tra file system

**df - Disk space:**
```bash
df -h                   # Human readable
df -i                   # Inodes
```

**du - Disk usage:**
```bash
du -sh directory/
du -h --max-depth=1
```

**stat - File information:**
```bash
stat file.txt
```

### 3. Common errors

**EMFILE - Too many open files:**
```c
if (errno == EMFILE) {
    // Process limit reached
    // Increase with ulimit -n
}
```

**ENFILE - System-wide file table overflow:**
```c
if (errno == ENFILE) {
    // System limit reached
    // Check /proc/sys/fs/file-max
}
```

**ENOSPC - No space left on device:**
```c
if (errno == ENOSPC) {
    // Disk full or inode exhausted
}
```

**ETXTBSY - Text file busy:**
```c
if (errno == ETXTBSY) {
    // Cannot modify executable being run
}
```

---

## Tài liệu tham khảo

### Man pages

```bash
man 2 open      # System calls
man 3 fopen     # Library functions
man 7 inotify   # Overview
```

### Sách và tài liệu

- **The Linux Programming Interface** - Michael Kerrisk
- **Advanced Programming in the UNIX Environment** - Stevens & Rago
- **Linux System Programming** - Robert Love
- Linux kernel documentation: `Documentation/filesystems/`

### Online resources

- man7.org - Linux man pages online
- kernel.org - Linux kernel source và documentation
- lwn.net - Linux Weekly News (kernel development)

---

## Kết luận

File management là nền tảng của Linux programming. Các khái niệm quan trọng:

1. File descriptors và basic I/O operations
2. File locking cho synchronization
3. Memory-mapped files cho performance
4. Directory traversal
5. Inotify cho file system monitoring
6. VFS architecture trong kernel

Hiểu rõ các concepts này giúp viết code hiệu quả, an toàn và portable trên Linux.

---

## Liên hệ và đóng góp

Nếu có vấn đề hoặc muốn đóng góp, vui lòng tạo issue hoặc pull request trên repository.



