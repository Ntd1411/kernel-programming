/*
 * directory_walk.c - Duyệt thư mục và liệt kê file
 * 
 * Minh họa: opendir, readdir, closedir, stat
 * 
 * Biên dịch: gcc -o directory_walk directory_walk.c
 * Chạy: ./directory_walk
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>

#define MAX_PATH 512

void print_menu(void) {
    printf("\n=== MENU DIRECTORY WALK ===\n");
    printf("1. Liệt kê đơn giản\n");
    printf("2. Liệt kê chi tiết (ls -l)\n");
    printf("3. Thống kê thư mục\n");
    printf("4. Hiển thị dạng tree\n");
    printf("5. Tìm file theo tên\n");
    printf("6. Tìm file lớn\n");
    printf("0. Thoát\n");
    printf("Chọn: ");
}

void print_permissions(mode_t mode) {
    printf((S_ISDIR(mode)) ? "d" : "-");
    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");
    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");
    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
}

void print_file_type(mode_t mode) {
    if (S_ISREG(mode)) printf("Regular file");
    else if (S_ISDIR(mode)) printf("Directory");
    else if (S_ISLNK(mode)) printf("Symbolic link");
    else if (S_ISCHR(mode)) printf("Character device");
    else if (S_ISBLK(mode)) printf("Block device");
    else if (S_ISFIFO(mode)) printf("FIFO/pipe");
    else if (S_ISSOCK(mode)) printf("Socket");
    else printf("Unknown");
}

void list_directory_simple(const char *path) {
    DIR *dir;
    struct dirent *entry;
    
    printf("\n=== Liệt kê đơn giản: %s ===\n", path);
    
    dir = opendir(path);
    if (dir == NULL) {
        perror("opendir");
        return;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        printf("%s\n", entry->d_name);
    }
    
    closedir(dir);
}

void list_directory_detailed(const char *path) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char full_path[1024];
    struct passwd *pwd;
    struct group *grp;
    char time_str[64];
    
    printf("\n=== Liệt kê chi tiết (ls -l style): %s ===\n", path);
    
    dir = opendir(path);
    if (dir == NULL) {
        perror("opendir");
        return;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        if (lstat(full_path, &file_stat) == -1) {
            perror("lstat");
            continue;
        }
        
        print_permissions(file_stat.st_mode);
        printf(" %2ld", file_stat.st_nlink);
        
        pwd = getpwuid(file_stat.st_uid);
        grp = getgrgid(file_stat.st_gid);
        
        printf(" %8s", pwd ? pwd->pw_name : "?");
        printf(" %8s", grp ? grp->gr_name : "?");
        printf(" %8ld", file_stat.st_size);
        
        strftime(time_str, sizeof(time_str), "%b %d %H:%M", 
                 localtime(&file_stat.st_mtime));
        printf(" %s", time_str);
        
        printf(" %s", entry->d_name);
        
        if (S_ISLNK(file_stat.st_mode)) {
            char link_target[1024];
            ssize_t len = readlink(full_path, link_target, sizeof(link_target) - 1);
            if (len != -1) {
                link_target[len] = '\0';
                printf(" -> %s", link_target);
            }
        }
        
        printf("\n");
    }
    
    closedir(dir);
}

void count_directory(const char *path) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char full_path[1024];
    int files = 0, dirs = 0, links = 0, other = 0;
    off_t total_size = 0;
    
    printf("\n=== Thống kê thư mục: %s ===\n", path);
    
    dir = opendir(path);
    if (dir == NULL) {
        perror("opendir");
        return;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
            
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        if (lstat(full_path, &file_stat) == -1) {
            continue;
        }
        
        if (S_ISREG(file_stat.st_mode)) {
            files++;
            total_size += file_stat.st_size;
        }
        else if (S_ISDIR(file_stat.st_mode)) dirs++;
        else if (S_ISLNK(file_stat.st_mode)) links++;
        else other++;
    }
    
    closedir(dir);
    
    printf("Regular files: %d\n", files);
    printf("Directories:   %d\n", dirs);
    printf("Symbolic links: %d\n", links);
    printf("Other:         %d\n", other);
    printf("Total size:    %ld bytes (%.2f MB)\n", 
           total_size, total_size / (1024.0 * 1024.0));
}

void walk_directory_recursive(const char *path, int depth) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char full_path[1024];
    
    if (depth > 10) {
        printf("Max depth reached\n");
        return;
    }
    
    dir = opendir(path);
    if (dir == NULL) {
        return;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
            
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        for (int i = 0; i < depth; i++) printf("  ");
        
        if (lstat(full_path, &file_stat) == -1) {
            printf("%s [error]\n", entry->d_name);
            continue;
        }
        
        if (S_ISDIR(file_stat.st_mode)) {
            printf("[DIR]  %s\n", entry->d_name);
            walk_directory_recursive(full_path, depth + 1);
        } else {
            printf("[FILE] %s (%ld bytes)\n", entry->d_name, file_stat.st_size);
        }
    }
    
    closedir(dir);
}

void find_by_name(const char *path, const char *pattern, int depth) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char full_path[1024];
    
    if (depth > 10) return;
    
    dir = opendir(path);
    if (dir == NULL) return;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
            
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        if (strstr(entry->d_name, pattern) != NULL) {
            printf("Found: %s\n", full_path);
        }
        
        if (lstat(full_path, &file_stat) != -1 && S_ISDIR(file_stat.st_mode)) {
            find_by_name(full_path, pattern, depth + 1);
        }
    }
    
    closedir(dir);
}

void find_large_files(const char *path, off_t min_size, int depth) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char full_path[1024];
    
    if (depth > 10) return;
    
    dir = opendir(path);
    if (dir == NULL) return;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
            
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        if (lstat(full_path, &file_stat) == -1) continue;
        
        if (S_ISREG(file_stat.st_mode) && file_stat.st_size >= min_size) {
            printf("%10ld bytes: %s\n", file_stat.st_size, full_path);
        }
        
        if (S_ISDIR(file_stat.st_mode)) {
            find_large_files(full_path, min_size, depth + 1);
        }
    }
    
    closedir(dir);
}

int main(void) {
    int choice;
    char dir_path[MAX_PATH];
    char pattern[256];
    off_t min_size;
    
    printf("=== DIRECTORY WALK DEMO ===\n");
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
                printf("Nhập đường dẫn thư mục: ");
                if (fgets(dir_path, sizeof(dir_path), stdin) == NULL) break;
                dir_path[strcspn(dir_path, "\n")] = 0;
                list_directory_simple(dir_path);
                break;
                
            case 2:
                printf("Nhập đường dẫn thư mục: ");
                if (fgets(dir_path, sizeof(dir_path), stdin) == NULL) break;
                dir_path[strcspn(dir_path, "\n")] = 0;
                list_directory_detailed(dir_path);
                break;
                
            case 3:
                printf("Nhập đường dẫn thư mục: ");
                if (fgets(dir_path, sizeof(dir_path), stdin) == NULL) break;
                dir_path[strcspn(dir_path, "\n")] = 0;
                count_directory(dir_path);
                break;
                
            case 4:
                printf("Nhập đường dẫn thư mục: ");
                if (fgets(dir_path, sizeof(dir_path), stdin) == NULL) break;
                dir_path[strcspn(dir_path, "\n")] = 0;
                printf("\n=== Directory tree: %s ===\n", dir_path);
                walk_directory_recursive(dir_path, 0);
                break;

            case 5:
                printf("Nhập mẫu tên file cần tìm: ");
                if (fgets(pattern, sizeof(pattern), stdin) == NULL) break;
                pattern[strcspn(pattern, "\n")] = 0;
                printf("Nhập đường dẫn thư mục: ");
                if (fgets(dir_path, sizeof(dir_path), stdin) == NULL) break;
                dir_path[strcspn(dir_path, "\n")] = 0;
                printf("\n=== Tìm file theo tên: %s ===\n", pattern);
                find_by_name(dir_path, pattern, 0);
                break;

            case 6:
                printf("Nhập kích thước tối thiểu (bytes): ");
                if (scanf("%ld", &min_size) != 1) {
                    while (getchar() != '\n');
                    printf("Lỗi: Vui lòng nhập số!\n");
                    break;
                }
                while (getchar() != '\n');
                printf("Nhập đường dẫn thư mục: ");
                if (fgets(dir_path, sizeof(dir_path), stdin) == NULL) break;
                dir_path[strcspn(dir_path, "\n")] = 0;
                printf("\n=== File lớn hơn %ld bytes ===\n", min_size);
                find_large_files(dir_path, min_size, 0);
                break;

            default:
                printf("Lựa chọn không hợp lệ. Vui lòng chọn lại.\n");
                break;
        }
    }

    return 0;
}
