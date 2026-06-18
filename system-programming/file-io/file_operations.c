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
#define MAX_INPUT 256

void print_menu(void) {
    printf("\n=== MENU FILE OPERATIONS ===\n");
    printf("1. Ghi file (write)\n");
    printf("2. Đọc file (read)\n");
    printf("3. Thêm vào file (append)\n");
    printf("4. Sao chép file (copy)\n");
    printf("5. Thông tin file (stat)\n");
    printf("0. Thoát\n");
    printf("Chọn: ");
}

int write_file(const char *filename, const char *content) {
    int fd;
    ssize_t bytes_written;
    
    printf("\n=== Ghi File ===\n");
    printf("File: %s\n", filename);
    
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
    
    printf("\n=== Đọc File ===\n");
    
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
    
    printf("\n=== Thêm vào File ===\n");
    printf("File: %s\n", filename);
    
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
    
    printf("\n=== Sao Chép File ===\n");
    printf("Từ: %s\n", src);
    printf("Đến: %s\n", dst);
    
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
    
    printf("\n=== Thông Tin File ===\n");
    
    if (stat(filename, &file_stat) == -1) {
        perror("stat");
        return -1;
    }
    
    printf("File: %s\n", filename);
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

int main(void) {
    int choice;
    char filename[MAX_INPUT];
    char content[MAX_INPUT];
    char src_file[MAX_INPUT];
    char dst_file[MAX_INPUT];
    
    printf("=== FILE OPERATIONS DEMO ===\n");
    printf("PID: %d\n", getpid());
    
    while (1) {
        print_menu();
        
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            printf("Lỗi: Vui lòng nhập số!\n");
            continue;
        }
        while (getchar() != '\n');
        
        if (choice == 0) {
            printf("\nThoát chương trình.\n");
            break;
        }
        
        switch (choice) {
            case 1:
                printf("Nhập tên file: ");
                if (fgets(filename, sizeof(filename), stdin) == NULL) break;
                filename[strcspn(filename, "\n")] = 0;
                
                printf("Nhập nội dung: ");
                if (fgets(content, sizeof(content), stdin) == NULL) break;
                content[strcspn(content, "\n")] = 0;
                
                write_file(filename, content);
                break;
                
            case 2:
                printf("Nhập tên file: ");
                if (fgets(filename, sizeof(filename), stdin) == NULL) break;
                filename[strcspn(filename, "\n")] = 0;
                
                read_file(filename);
                break;
                
            case 3:
                printf("Nhập tên file: ");
                if (fgets(filename, sizeof(filename), stdin) == NULL) break;
                filename[strcspn(filename, "\n")] = 0;
                
                printf("Nhập nội dung thêm vào: ");
                if (fgets(content, sizeof(content), stdin) == NULL) break;
                content[strcspn(content, "\n")] = 0;
                
                append_file(filename, content);
                break;
                
            case 4:
                printf("Nhập file nguồn: ");
                if (fgets(src_file, sizeof(src_file), stdin) == NULL) break;
                src_file[strcspn(src_file, "\n")] = 0;
                
                printf("Nhập file đích: ");
                if (fgets(dst_file, sizeof(dst_file), stdin) == NULL) break;
                dst_file[strcspn(dst_file, "\n")] = 0;
                
                copy_file(src_file, dst_file);
                break;
                
            case 5:
                printf("Nhập tên file: ");
                if (fgets(filename, sizeof(filename), stdin) == NULL) break;
                filename[strcspn(filename, "\n")] = 0;
                
                show_stat(filename);
                break;
                
            default:
                printf("Lỗi: Lựa chọn không hợp lệ!\n");
                break;
        }
        
        printf("\nNhấn Enter để tiếp tục...");
        getchar();
    }
    
    return 0;
}
