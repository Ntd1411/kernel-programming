/*
 * inotify_example.c - Giám sát thay đổi file/directory với inotify
 * 
 * Minh họa: inotify_init, inotify_add_watch, inotify_rm_watch
 * 
 * Biên dịch: gcc -o inotify_example inotify_example.c
 * Chạy: ./inotify_example
 * 
 * Chương trình sử dụng menu tương tác để chọn chức năng:
 * 1. Giám sát 1 đường dẫn
 * 2. Giám sát nhiều đường dẫn cùng lúc
 * 3. Giám sát với thời gian timeout
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/inotify.h>
#include <sys/select.h>
#include <limits.h>

#define MAX_EVENTS 1024
#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (MAX_EVENTS * (EVENT_SIZE + NAME_MAX + 1))

void print_event_mask(uint32_t mask) {
    printf("Events: ");
    
    if (mask & IN_ACCESS) printf("ACCESS ");
    if (mask & IN_ATTRIB) printf("ATTRIB ");
    if (mask & IN_CLOSE_WRITE) printf("CLOSE_WRITE ");
    if (mask & IN_CLOSE_NOWRITE) printf("CLOSE_NOWRITE ");
    if (mask & IN_CREATE) printf("CREATE ");
    if (mask & IN_DELETE) printf("DELETE ");
    if (mask & IN_DELETE_SELF) printf("DELETE_SELF ");
    if (mask & IN_MODIFY) printf("MODIFY ");
    if (mask & IN_MOVE_SELF) printf("MOVE_SELF ");
    if (mask & IN_MOVED_FROM) printf("MOVED_FROM ");
    if (mask & IN_MOVED_TO) printf("MOVED_TO ");
    if (mask & IN_OPEN) printf("OPEN ");
    if (mask & IN_IGNORED) printf("IGNORED ");
    if (mask & IN_ISDIR) printf("ISDIR ");
    
    printf("\n");
}

void monitor_path(const char *path, uint32_t mask) {
    int fd, wd;
    char buffer[EVENT_BUF_LEN];
    
    printf("\n=== Giám sát: %s ===\n", path);
    printf("Nhấn Ctrl+C để dừng\n\n");
    
    fd = inotify_init();
    if (fd < 0) {
        perror("inotify_init");
        return;
    }
    
    wd = inotify_add_watch(fd, path, mask);
    if (wd < 0) {
        perror("inotify_add_watch");
        close(fd);
        return;
    }
    
    printf("Watch descriptor: %d\n", wd);
    printf("Đang theo dõi...\n\n");
    
    while (1) {
        int length = read(fd, buffer, EVENT_BUF_LEN);
        if (length < 0) {
            perror("read");
            break;
        }
        
        int i = 0;
        while (i < length) {
            struct inotify_event *event = (struct inotify_event *)&buffer[i];
            
            if (event->len) {
                printf("\n[%s] ", path);
                if (event->mask & IN_ISDIR) {
                    printf("Directory: %s\n", event->name);
                } else {
                    printf("File: %s\n", event->name);
                }
                print_event_mask(event->mask);
            } else {
                printf("\n[%s]\n", path);
                print_event_mask(event->mask);
            }
            
            i += EVENT_SIZE + event->len;
        }
    }
    
    inotify_rm_watch(fd, wd);
    close(fd);
}

void monitor_multiple_paths(char *paths[], int count) {
    int fd;
    int wds[count];
    char buffer[EVENT_BUF_LEN];
    
    printf("\n=== Giám sát nhiều đường dẫn ===\n");
    
    fd = inotify_init();
    if (fd < 0) {
        perror("inotify_init");
        return;
    }
    
    uint32_t mask = IN_CREATE | IN_DELETE | IN_MODIFY | 
                    IN_MOVED_FROM | IN_MOVED_TO;
    
    for (int i = 0; i < count; i++) {
        wds[i] = inotify_add_watch(fd, paths[i], mask);
        if (wds[i] < 0) {
            perror("inotify_add_watch");
            printf("Không thể watch: %s\n", paths[i]);
        } else {
            printf("Watching [%d]: %s\n", wds[i], paths[i]);
        }
    }
    
    printf("\nĐang theo dõi... (Nhấn Ctrl+C để dừng)\n\n");
    
    while (1) {
        int length = read(fd, buffer, EVENT_BUF_LEN);
        if (length < 0) {
            perror("read");
            break;
        }
        
        int i = 0;
        while (i < length) {
            struct inotify_event *event = (struct inotify_event *)&buffer[i];
            
            for (int j = 0; j < count; j++) {
                if (event->wd == wds[j]) {
                    printf("\n[Watch %d: %s]\n", event->wd, paths[j]);
                    break;
                }
            }
            
            if (event->len) {
                printf("  %s %s\n", 
                       (event->mask & IN_ISDIR) ? "DIR " : "FILE",
                       event->name);
            }
            print_event_mask(event->mask);
            
            i += EVENT_SIZE + event->len;
        }
    }
    
    for (int i = 0; i < count; i++) {
        if (wds[i] >= 0) {
            inotify_rm_watch(fd, wds[i]);
        }
    }
    close(fd);
}

void monitor_with_timeout(const char *path, int timeout_sec) {
    int fd, wd;
    char buffer[EVENT_BUF_LEN];
    fd_set fds;
    struct timeval tv;
    
    printf("\n=== Giám sát với timeout %d giây ===\n", timeout_sec);
    
    fd = inotify_init();
    if (fd < 0) {
        perror("inotify_init");
        return;
    }
    
    uint32_t mask = IN_ALL_EVENTS;
    wd = inotify_add_watch(fd, path, mask);
    if (wd < 0) {
        perror("inotify_add_watch");
        close(fd);
        return;
    }
    
    printf("Watching: %s\n", path);
    printf("Timeout sau %d giây nếu không có event\n\n", timeout_sec);
    
    while (1) {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        
        tv.tv_sec = timeout_sec;
        tv.tv_usec = 0;
        
        int ret = select(fd + 1, &fds, NULL, NULL, &tv);
        
        if (ret < 0) {
            perror("select");
            break;
        } else if (ret == 0) {
            printf("Timeout! Không có event trong %d giây\n", timeout_sec);
            break;
        }
        
        int length = read(fd, buffer, EVENT_BUF_LEN);
        if (length < 0) {
            perror("read");
            break;
        }
        
        int i = 0;
        while (i < length) {
            struct inotify_event *event = (struct inotify_event *)&buffer[i];
            
            if (event->len) {
                printf("Event: %s\n", event->name);
            }
            print_event_mask(event->mask);
            printf("\n");
            
            i += EVENT_SIZE + event->len;
        }
    }
    
    inotify_rm_watch(fd, wd);
    close(fd);
}

void print_menu(void) {
    printf("\n=== CHƯƠNG TRÌNH GIÁM SÁT FILE VỚI INOTIFY ===\n");
    printf("1. Giám sát 1 đường dẫn\n");
    printf("2. Giám sát nhiều đường dẫn\n");
    printf("3. Giám sát với timeout\n");
    printf("0. Thoát\n");
    printf("=============================================\n");
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
    char path[PATH_MAX];
    char paths[10][PATH_MAX];
    int path_count;
    int timeout;
    
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
                printf("\n--- Giám sát 1 đường dẫn ---\n");
                get_input_string("Nhập đường dẫn cần giám sát: ", path, sizeof(path));
                
                if (strlen(path) == 0) {
                    printf("Lỗi: Đường dẫn không được để trống\n");
                    break;
                }
                
                uint32_t mask = IN_CREATE | IN_DELETE | IN_MODIFY | 
                                IN_MOVED_FROM | IN_MOVED_TO | 
                                IN_CLOSE_WRITE | IN_OPEN;
                monitor_path(path, mask);
                break;
                
            case 2:
                printf("\n--- Giám sát nhiều đường dẫn ---\n");
                printf("Nhập số lượng đường dẫn (tối đa 10): ");
                
                if (scanf("%d", &path_count) != 1 || path_count < 1 || path_count > 10) {
                    printf("Lỗi: Số lượng không hợp lệ\n");
                    while (getchar() != '\n');
                    break;
                }
                getchar();
                
                char *path_ptrs[10];
                for (int i = 0; i < path_count; i++) {
                    char prompt[100];
                    snprintf(prompt, sizeof(prompt), "Đường dẫn %d: ", i + 1);
                    get_input_string(prompt, paths[i], sizeof(paths[i]));
                    path_ptrs[i] = paths[i];
                }
                
                monitor_multiple_paths(path_ptrs, path_count);
                break;
                
            case 3:
                printf("\n--- Giám sát với timeout ---\n");
                get_input_string("Nhập đường dẫn cần giám sát: ", path, sizeof(path));
                
                if (strlen(path) == 0) {
                    printf("Lỗi: Đường dẫn không được để trống\n");
                    break;
                }
                
                printf("Nhập thời gian timeout (giây): ");
                if (scanf("%d", &timeout) != 1 || timeout <= 0) {
                    printf("Lỗi: Thời gian không hợp lệ\n");
                    while (getchar() != '\n');
                    break;
                }
                getchar();
                
                monitor_with_timeout(path, timeout);
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
