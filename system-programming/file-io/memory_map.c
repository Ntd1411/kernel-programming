/*
 * memory_map.c - Memory-mapped I/O với mmap
 * 
 * Minh họa cách map file vào memory và thao tác trực tiếp
 * 
 * Biên dịch: gcc -o memory_map memory_map.c
 * Chạy: ./memory_map
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>

#define FILE_SIZE (4 * 1024)  // 4 KB

void demo_read_mmap(const char *filename) {
    int fd;
    struct stat sb;
    char *mapped;
    
    printf("\n=== Demo: Đọc file bằng mmap ===\n");
    
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return;
    }
    
    if (fstat(fd, &sb) == -1) {
        perror("fstat");
        close(fd);
        return;
    }
    
    if (sb.st_size == 0) {
        printf("File rỗng\n");
        close(fd);
        return;
    }
    
    printf("File size: %ld bytes\n", sb.st_size);
    
    mapped = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return;
    }
    
    printf("File đã được map vào memory tại: %p\n", mapped);
    printf("\nNội dung file:\n");
    printf("----------------------------------------\n");
    
    for (off_t i = 0; i < sb.st_size; i++) {
        putchar(mapped[i]);
    }
    
    printf("\n----------------------------------------\n");
    
    if (munmap(mapped, sb.st_size) == -1) {
        perror("munmap");
    }
    
    close(fd);
}

void demo_write_mmap(const char *filename) {
    int fd;
    char *mapped;
    const char *text = "Hello from mmap!\nThis is memory-mapped I/O.\n";
    
    printf("\n=== Demo: Ghi file bằng mmap ===\n");
    
    fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return;
    }
    
    if (ftruncate(fd, FILE_SIZE) == -1) {
        perror("ftruncate");
        close(fd);
        return;
    }
    
    mapped = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return;
    }
    
    printf("Ghi dữ liệu vào memory-mapped region...\n");
    strcpy(mapped, text);
    
    printf("Dữ liệu đã ghi: %s", mapped);
    
    if (msync(mapped, FILE_SIZE, MS_SYNC) == -1) {
        perror("msync");
    }
    
    printf("Đã sync vào disk\n");
    
    if (munmap(mapped, FILE_SIZE) == -1) {
        perror("munmap");
    }
    
    close(fd);
}

void demo_shared_memory(void) {
    char *mapped;
    pid_t pid;
    
    printf("\n=== Demo: Shared memory giữa parent và child ===\n");
    
    mapped = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, 
                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap");
        return;
    }
    
    strcpy(mapped, "Message from parent");
    
    pid = fork();
    if (pid == -1) {
        perror("fork");
        munmap(mapped, FILE_SIZE);
        return;
    }
    
    if (pid == 0) {
        printf("Child process (PID %d)\n", getpid());
        printf("Child đọc: '%s'\n", mapped);
        
        strcpy(mapped, "Message from child");
        printf("Child đã ghi message mới\n");
        
        exit(0);
    } else {
        printf("Parent process (PID %d)\n", getpid());
        printf("Parent ghi: '%s'\n", mapped);
        
        sleep(1);
        
        printf("Parent đọc sau khi child ghi: '%s'\n", mapped);
        
        wait(NULL);
    }
    
    if (munmap(mapped, FILE_SIZE) == -1) {
        perror("munmap");
    }
}

void demo_copy_file(const char *src, const char *dst) {
    int src_fd, dst_fd;
    struct stat sb;
    char *src_map, *dst_map;
    
    printf("\n=== Demo: Copy file bằng mmap ===\n");
    
    src_fd = open(src, O_RDONLY);
    if (src_fd == -1) {
        perror("open source");
        return;
    }
    
    if (fstat(src_fd, &sb) == -1) {
        perror("fstat");
        close(src_fd);
        return;
    }
    
    if (sb.st_size == 0) {
        printf("Source file rỗng\n");
        close(src_fd);
        return;
    }
    
    dst_fd = open(dst, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (dst_fd == -1) {
        perror("open destination");
        close(src_fd);
        return;
    }
    
    if (ftruncate(dst_fd, sb.st_size) == -1) {
        perror("ftruncate");
        close(src_fd);
        close(dst_fd);
        return;
    }
    
    src_map = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
    if (src_map == MAP_FAILED) {
        perror("mmap source");
        close(src_fd);
        close(dst_fd);
        return;
    }
    
    dst_map = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, dst_fd, 0);
    if (dst_map == MAP_FAILED) {
        perror("mmap destination");
        munmap(src_map, sb.st_size);
        close(src_fd);
        close(dst_fd);
        return;
    }
    
    printf("Copying %ld bytes bằng mmap...\n", sb.st_size);
    memcpy(dst_map, src_map, sb.st_size);
    
    printf("Copy hoàn tất!\n");
    
    munmap(src_map, sb.st_size);
    munmap(dst_map, sb.st_size);
    close(src_fd);
    close(dst_fd);
}

void print_usage(const char *prog) {
    printf("Sử dụng:\n");
    printf("  %s read <file>       - Đọc file bằng mmap\n", prog);
    printf("  %s write <file>      - Ghi file bằng mmap\n", prog);
    printf("  %s shared            - Demo shared memory\n", prog);
    printf("  %s copy <src> <dst>  - Copy file bằng mmap\n", prog);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char *cmd = argv[1];
    
    if (strcmp(cmd, "read") == 0) {
        if (argc != 3) {
            printf("Sử dụng: %s read <file>\n", argv[0]);
            return 1;
        }
        demo_read_mmap(argv[2]);
    }
    else if (strcmp(cmd, "write") == 0) {
        if (argc != 3) {
            printf("Sử dụng: %s write <file>\n", argv[0]);
            return 1;
        }
        demo_write_mmap(argv[2]);
    }
    else if (strcmp(cmd, "shared") == 0) {
        demo_shared_memory();
    }
    else if (strcmp(cmd, "copy") == 0) {
        if (argc != 4) {
            printf("Sử dụng: %s copy <src> <dst>\n", argv[0]);
            return 1;
        }
        demo_copy_file(argv[2], argv[3]);
    }
    else {
        printf("Lệnh không hợp lệ: %s\n", cmd);
        print_usage(argv[0]);
        return 1;
    }
    
    return 0;
}
