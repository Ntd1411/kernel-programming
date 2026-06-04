/*
 * directory_walk.c - Duyệt thư mục và liệt kê file
 * 
 * Minh họa: opendir, readdir, closedir, stat
 * 
 * Biên dịch: gcc -o directory_walk directory_walk.c
 * Chạy: ./directory_walk <directory>
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

void print_usage(const char *prog) {
    printf("Sử dụng:\n");
    printf("  %s list <dir>           - Liệt kê đơn giản\n", prog);
    printf("  %s detail <dir>         - Liệt kê chi tiết\n", prog);
    printf("  %s count <dir>          - Thống kê\n", prog);
    printf("  %s tree <dir>           - Hiển thị dạng tree\n", prog);
    printf("  %s find <dir> <pattern> - Tìm file theo tên\n", prog);
    printf("  %s large <dir> <size>   - Tìm file lớn hơn size (bytes)\n", prog);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char *cmd = argv[1];
    const char *path = argv[2];
    
    if (strcmp(cmd, "list") == 0) {
        list_directory_simple(path);
    }
    else if (strcmp(cmd, "detail") == 0) {
        list_directory_detailed(path);
    }
    else if (strcmp(cmd, "count") == 0) {
        count_directory(path);
    }
    else if (strcmp(cmd, "tree") == 0) {
        printf("\n=== Directory tree: %s ===\n", path);
        walk_directory_recursive(path, 0);
    }
    else if (strcmp(cmd, "find") == 0) {
        if (argc != 4) {
            printf("Sử dụng: %s find <dir> <pattern>\n", argv[0]);
            return 1;
        }
        printf("\n=== Tìm kiếm '%s' trong %s ===\n", argv[3], path);
        find_by_name(path, argv[3], 0);
    }
    else if (strcmp(cmd, "large") == 0) {
        if (argc != 4) {
            printf("Sử dụng: %s large <dir> <size>\n", argv[0]);
            return 1;
        }
        off_t min_size = atoll(argv[3]);
        printf("\n=== File lớn hơn %ld bytes trong %s ===\n", min_size, path);
        find_large_files(path, min_size, 0);
    }
    else {
        printf("Lệnh không hợp lệ: %s\n", cmd);
        print_usage(argv[0]);
        return 1;
    }
    
    return 0;
}
