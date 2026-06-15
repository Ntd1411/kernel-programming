/*
 * inotify_example.c - Giám sát thay đổi file/directory với inotify
 * 
 * Minh họa: inotify_init, inotify_add_watch, inotify_rm_watch
 * 
 * Biên dịch: gcc -o inotify_example inotify_example.c
 * Chạy: ./inotify_example <path1> [path2] ...
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

void print_usage(const char *prog) {
    printf("Sử dụng:\n");
    printf("  %s <path>                  - Giám sát 1 đường dẫn\n", prog);
    printf("  %s -m <path1> <path2> ...  - Giám sát nhiều đường dẫn\n", prog);
    printf("  %s -t <path> <seconds>     - Giám sát với timeout\n", prog);
    printf("\nVí dụ:\n");
    printf("  %s /tmp\n", prog);
    printf("  %s -m /tmp /var/log\n", prog);
    printf("  %s -t /home/user 30\n", prog);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    if (strcmp(argv[1], "-m") == 0) {
        if (argc < 3) {
            printf("Lỗi: Cần ít nhất 1 đường dẫn\n");
            return 1;
        }
        monitor_multiple_paths(&argv[2], argc - 2);
    }
    else if (strcmp(argv[1], "-t") == 0) {
        if (argc != 4) {
            printf("Sử dụng: %s -t <path> <seconds>\n", argv[0]);
            return 1;
        }
        int timeout = atoi(argv[3]);
        monitor_with_timeout(argv[2], timeout);
    }
    else {
        uint32_t mask = IN_CREATE | IN_DELETE | IN_MODIFY | 
                        IN_MOVED_FROM | IN_MOVED_TO | 
                        IN_CLOSE_WRITE | IN_OPEN;
        monitor_path(argv[1], mask);
    }
    
    return 0;
}
