/*
 * memory_map.c - Memory-mapped I/O với mmap
 * 
 * Minh họa cách map file vào memory và thao tác trực tiếp
 * 
 * Biên dịch: gcc -o memory_map memory_map.c
 * Chạy: ./memory_map
 * 
 * Chương trình sử dụng menu tương tác để chọn chức năng:
 * 1. Đọc file bằng mmap
 * 2. Ghi file bằng mmap
 * 3. Demo shared memory giữa parent và child
 * 4. Copy file bằng mmap
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
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

void print_menu(void) {
    printf("\n=== CHƯƠNG TRÌNH MEMORY-MAPPED I/O ===\n");
    printf("1. Đọc file bằng mmap\n");
    printf("2. Ghi file bằng mmap\n");
    printf("3. Demo shared memory (parent-child)\n");
    printf("4. Copy file bằng mmap\n");
    printf("0. Thoát\n");
    printf("======================================\n");
    printf("Chọn chức năng: ");
}

void get_input_string(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);
    if (fgets(buffer, size, stdin) != NULL) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
    }
}

int main(int argc, char *argv[]) {
    int choice;
    char filename[256];
    char src_file[256];
    char dst_file[256];
    
    while (1) {
        print_menu();
        
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            printf("Lỗi: Vui lòng nhập số\n");
            continue;
        }
        getchar();
        
        switch (choice) {
            case 1:
                printf("\n--- Đọc file bằng mmap ---\n");
                get_input_string("Nhập đường dẫn file cần đọc: ", filename, sizeof(filename));
                
                if (strlen(filename) == 0) {
                    printf("Lỗi: Đường dẫn không được để trống\n");
                    break;
                }
                
                demo_read_mmap(filename);
                break;
                
            case 2:
                printf("\n--- Ghi file bằng mmap ---\n");
                get_input_string("Nhập đường dẫn file cần ghi: ", filename, sizeof(filename));
                
                if (strlen(filename) == 0) {
                    printf("Lỗi: Đường dẫn không được để trống\n");
                    break;
                }
                
                demo_write_mmap(filename);
                break;
                
            case 3:
                demo_shared_memory();
                break;
                
            case 4:
                printf("\n--- Copy file bằng mmap ---\n");
                get_input_string("Nhập đường dẫn file nguồn: ", src_file, sizeof(src_file));
                
                if (strlen(src_file) == 0) {
                    printf("Lỗi: Đường dẫn nguồn không được để trống\n");
                    break;
                }
                
                get_input_string("Nhập đường dẫn file đích: ", dst_file, sizeof(dst_file));
                
                if (strlen(dst_file) == 0) {
                    printf("Lỗi: Đường dẫn đích không được để trống\n");
                    break;
                }
                
                demo_copy_file(src_file, dst_file);
                break;
                
            case 0:
                printf("\nKết thúc chương trình. Tạm biệt!\n");
                return 0;
                
            default:
                printf("Lỗi: Lựa chọn không hợp lệ. Vui lòng chọn lại.\n");
                break;
        }
    }
    
    return 0;
}
