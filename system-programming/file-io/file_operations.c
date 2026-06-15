/*
 * file_operations.c - Các thao tác file cơ bản với system calls
 * 
 * Minh họa: open, read, write, close, lseek
 * 
 * Biên dịch: gcc -o file_operations file_operations.c
 * Chạy: ./file_operations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

void print_usage(const char *prog_name) {
    printf("Sử dụng:\n");
    printf("  %s write <file> <content>   - Ghi nội dung vào file\n", prog_name);
    printf("  %s read <file>              - Đọc nội dung file\n", prog_name);
    printf("  %s append <file> <content>  - Thêm nội dung vào cuối file\n", prog_name);
    printf("  %s copy <src> <dst>         - Sao chép file\n", prog_name);
    printf("  %s stat <file>              - Hiển thị thông tin file\n", prog_name);
}

int write_file(const char *filename, const char *content) {
    int fd;
    ssize_t bytes_written;
    
    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return -1;
    }
    
    bytes_written = write(fd, content, strlen(content));
    if (bytes_written == -1) {
        perror("write");
        close(fd);
        return -1;
    }
    
    printf("Đã ghi %zd bytes vào %s\n", bytes_written, filename);
    
    close(fd);
    return 0;
}

int read_file(const char *filename) {
    int fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return -1;
    }
    
    printf("Nội dung file %s:\n", filename);
    printf("----------------------------------------\n");
    
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0';
        printf("%s", buffer);
    }
    
    if (bytes_read == -1) {
        perror("read");
        close(fd);
        return -1;
    }
    
    printf("\n----------------------------------------\n");
    
    close(fd);
    return 0;
}

int append_file(const char *filename, const char *content) {
    int fd;
    ssize_t bytes_written;
    
    fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("open");
        return -1;
    }
    
    bytes_written = write(fd, content, strlen(content));
    if (bytes_written == -1) {
        perror("write");
        close(fd);
        return -1;
    }
    
    printf("Đã thêm %zd bytes vào %s\n", bytes_written, filename);
    
    close(fd);
    return 0;
}

int copy_file(const char *src, const char *dst) {
    int src_fd, dst_fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;
    off_t total_bytes = 0;
    
    src_fd = open(src, O_RDONLY);
    if (src_fd == -1) {
        perror("open source");
        return -1;
    }
    
    dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd == -1) {
        perror("open destination");
        close(src_fd);
        return -1;
    }
    
    while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
        bytes_written = write(dst_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            perror("write");
            close(src_fd);
            close(dst_fd);
            return -1;
        }
        total_bytes += bytes_written;
    }
    
    if (bytes_read == -1) {
        perror("read");
        close(src_fd);
        close(dst_fd);
        return -1;
    }
    
    printf("Đã sao chép %ld bytes từ %s sang %s\n", total_bytes, src, dst);
    
    close(src_fd);
    close(dst_fd);
    return 0;
}

int show_stat(const char *filename) {
    struct stat file_stat;
    
    if (stat(filename, &file_stat) == -1) {
        perror("stat");
        return -1;
    }
    
    printf("Thông tin file: %s\n", filename);
    printf("----------------------------------------\n");
    printf("Inode:          %ld\n", file_stat.st_ino);
    printf("Size:           %ld bytes\n", file_stat.st_size);
    printf("Blocks:         %ld\n", file_stat.st_blocks);
    printf("Block size:     %ld bytes\n", file_stat.st_blksize);
    printf("Hard links:     %ld\n", file_stat.st_nlink);
    printf("UID:            %d\n", file_stat.st_uid);
    printf("GID:            %d\n", file_stat.st_gid);
    
    printf("Permissions:    ");
    printf((S_ISDIR(file_stat.st_mode)) ? "d" : "-");
    printf((file_stat.st_mode & S_IRUSR) ? "r" : "-");
    printf((file_stat.st_mode & S_IWUSR) ? "w" : "-");
    printf((file_stat.st_mode & S_IXUSR) ? "x" : "-");
    printf((file_stat.st_mode & S_IRGRP) ? "r" : "-");
    printf((file_stat.st_mode & S_IWGRP) ? "w" : "-");
    printf((file_stat.st_mode & S_IXGRP) ? "x" : "-");
    printf((file_stat.st_mode & S_IROTH) ? "r" : "-");
    printf((file_stat.st_mode & S_IWOTH) ? "w" : "-");
    printf((file_stat.st_mode & S_IXOTH) ? "x" : "-");
    printf("\n");
    
    printf("Last access:    %s", ctime(&file_stat.st_atime));
    printf("Last modified:  %s", ctime(&file_stat.st_mtime));
    printf("Last status:    %s", ctime(&file_stat.st_ctime));
    
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char *command = argv[1];
    
    if (strcmp(command, "write") == 0) {
        if (argc != 4) {
            printf("Lỗi: Thiếu tham số\n");
            printf("Sử dụng: %s write <file> <content>\n", argv[0]);
            return 1;
        }
        return write_file(argv[2], argv[3]);
    }
    else if (strcmp(command, "read") == 0) {
        if (argc != 3) {
            printf("Lỗi: Thiếu tham số\n");
            printf("Sử dụng: %s read <file>\n", argv[0]);
            return 1;
        }
        return read_file(argv[2]);
    }
    else if (strcmp(command, "append") == 0) {
        if (argc != 4) {
            printf("Lỗi: Thiếu tham số\n");
            printf("Sử dụng: %s append <file> <content>\n", argv[0]);
            return 1;
        }
        return append_file(argv[2], argv[3]);
    }
    else if (strcmp(command, "copy") == 0) {
        if (argc != 4) {
            printf("Lỗi: Thiếu tham số\n");
            printf("Sử dụng: %s copy <src> <dst>\n", argv[0]);
            return 1;
        }
        return copy_file(argv[2], argv[3]);
    }
    else if (strcmp(command, "stat") == 0) {
        if (argc != 3) {
            printf("Lỗi: Thiếu tham số\n");
            printf("Sử dụng: %s stat <file>\n", argv[0]);
            return 1;
        }
        return show_stat(argv[2]);
    }
    else {
        printf("Lỗi: Lệnh không hợp lệ: %s\n", command);
        print_usage(argv[0]);
        return 1;
    }
    
    return 0;
}
