/*
 * file_locking.c - File locking với fcntl
 * 
 * Minh họa advisory locking để đồng bộ truy cập file
 * 
 * Biên dịch: gcc -o file_locking file_locking.c
 * Chạy: ./file_locking <file> <read|write>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>

void print_lock_info(struct flock *lock) {
    printf("Lock info:\n");
    printf("  Type: ");
    switch (lock->l_type) {
        case F_RDLCK:
            printf("Read lock\n");
            break;
        case F_WRLCK:
            printf("Write lock\n");
            break;
        case F_UNLCK:
            printf("Unlock\n");
            break;
    }
    printf("  Whence: %d\n", lock->l_whence);
    printf("  Start: %ld\n", lock->l_start);
    printf("  Length: %ld\n", lock->l_len);
    printf("  PID: %d\n", lock->l_pid);
}

int set_lock(int fd, int type, int whence, off_t start, off_t len, int wait) {
    struct flock lock;
    
    lock.l_type = type;
    lock.l_whence = whence;
    lock.l_start = start;
    lock.l_len = len;
    lock.l_pid = getpid();
    
    int cmd = wait ? F_SETLKW : F_SETLK;
    
    if (fcntl(fd, cmd, &lock) == -1) {
        if (errno == EACCES || errno == EAGAIN) {
            printf("File đã bị lock bởi process khác\n");
        } else {
            perror("fcntl");
        }
        return -1;
    }
    
    return 0;
}

int test_lock(int fd, int type, int whence, off_t start, off_t len) {
    struct flock lock;
    
    lock.l_type = type;
    lock.l_whence = whence;
    lock.l_start = start;
    lock.l_len = len;
    
    if (fcntl(fd, F_GETLK, &lock) == -1) {
        perror("fcntl F_GETLK");
        return -1;
    }
    
    if (lock.l_type == F_UNLCK) {
        printf("File có thể lock\n");
        return 0;
    } else {
        printf("File đã bị lock:\n");
        print_lock_info(&lock);
        return 1;
    }
}

void demo_read_lock(const char *filename) {
    int fd;
    char buffer[1024];
    ssize_t bytes_read;
    
    printf("\n=== Demo Read Lock ===\n");
    
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return;
    }
    
    printf("Đặt read lock trên file...\n");
    if (set_lock(fd, F_RDLCK, SEEK_SET, 0, 0, 1) == -1) {
        close(fd);
        return;
    }
    
    printf("Read lock thành công! (PID: %d)\n", getpid());
    printf("Đọc file...\n");
    
    bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("Nội dung: %s\n", buffer);
    }
    
    printf("Giữ lock trong 5 giây... (Thử chạy instance khác)\n");
    sleep(5);
    
    printf("Unlock file...\n");
    set_lock(fd, F_UNLCK, SEEK_SET, 0, 0, 1);
    
    close(fd);
}

void demo_write_lock(const char *filename) {
    int fd;
    const char *content = "Data written with write lock\n";
    
    printf("\n=== Demo Write Lock ===\n");
    
    fd = open(filename, O_WRONLY | O_CREAT, 0644);
    if (fd == -1) {
        perror("open");
        return;
    }
    
    printf("Kiểm tra xem file có thể lock không...\n");
    test_lock(fd, F_WRLCK, SEEK_SET, 0, 0);
    
    printf("Đặt write lock trên file...\n");
    if (set_lock(fd, F_WRLCK, SEEK_SET, 0, 0, 1) == -1) {
        close(fd);
        return;
    }
    
    printf("Write lock thành công! (PID: %d)\n", getpid());
    printf("Ghi file...\n");
    
    if (write(fd, content, strlen(content)) == -1) {
        perror("write");
    } else {
        printf("Đã ghi: %s", content);
    }
    
    printf("Giữ lock trong 10 giây... (Thử chạy instance khác)\n");
    sleep(10);
    
    printf("Unlock file...\n");
    set_lock(fd, F_UNLCK, SEEK_SET, 0, 0, 1);
    
    close(fd);
}

void demo_partial_lock(const char *filename) {
    int fd;
    
    printf("\n=== Demo Partial Lock ===\n");
    
    fd = open(filename, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        perror("open");
        return;
    }
    
    printf("Lock bytes 0-99...\n");
    if (set_lock(fd, F_WRLCK, SEEK_SET, 0, 100, 0) == 0) {
        printf("Lock thành công trên bytes 0-99\n");
    }
    
    printf("Lock bytes 200-299...\n");
    if (set_lock(fd, F_WRLCK, SEEK_SET, 200, 100, 0) == 0) {
        printf("Lock thành công trên bytes 200-299\n");
    }
    
    printf("Bytes 100-199 vẫn có thể lock bởi process khác\n");
    
    printf("Giữ lock trong 5 giây...\n");
    sleep(5);
    
    printf("Unlock tất cả...\n");
    set_lock(fd, F_UNLCK, SEEK_SET, 0, 0, 1);
    
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Sử dụng: %s <file> [read|write|partial]\n", argv[0]);
        printf("\nDemo:\n");
        printf("  Terminal 1: %s test.txt write\n", argv[0]);
        printf("  Terminal 2: %s test.txt read   (chạy trong khi T1 đang hold lock)\n", argv[0]);
        return 1;
    }
    
    const char *filename = argv[1];
    const char *mode = (argc > 2) ? argv[2] : "read";
    
    if (strcmp(mode, "read") == 0) {
        demo_read_lock(filename);
    }
    else if (strcmp(mode, "write") == 0) {
        demo_write_lock(filename);
    }
    else if (strcmp(mode, "partial") == 0) {
        demo_partial_lock(filename);
    }
    else {
        printf("Mode không hợp lệ: %s\n", mode);
        printf("Sử dụng: read, write, hoặc partial\n");
        return 1;
    }
    
    return 0;
}
