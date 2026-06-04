/*
 * thread_detach.c - Detached threads
 * 
 * Detached thread tự động giải phóng tài nguyên khi kết thúc
 * Không cần pthread_join()
 * 
 * Biên dịch: gcc -o thread_detach thread_detach.c -pthread
 * Chạy: ./thread_detach
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

void *detached_thread(void *arg) {
    int id = *(int *)arg;
    
    printf("Detached Thread %d: Bắt đầu (không cần join)\n", id);
    
    sleep(2);
    
    printf("Detached Thread %d: Kết thúc\n", id);
    
    free(arg);
    
    return NULL;
}

void *joinable_thread(void *arg) {
    int id = *(int *)arg;
    
    printf("Joinable Thread %d: Bắt đầu (cần join)\n", id);
    
    sleep(2);
    
    printf("Joinable Thread %d: Kết thúc\n", id);
    
    return NULL;
}

void *worker_thread(void *arg) {
    char *task = (char *)arg;
    
    printf("Worker: Thực hiện task '%s'\n", task);
    
    sleep(3);
    
    printf("Worker: Hoàn thành task '%s'\n", task);
    
    return NULL;
}

int main() {
    pthread_t thread1, thread2, thread3;
    pthread_attr_t attr;
    int id1 = 1, id2 = 2;
    int *id3;
    
    printf("=== Demo Detached vs Joinable Threads ===\n\n");
    
    // Tạo joinable thread (mặc định)
    printf("1. Tạo joinable thread\n");
    if (pthread_create(&thread1, NULL, joinable_thread, &id1) != 0) {
        perror("pthread_create failed");
        exit(EXIT_FAILURE);
    }
    
    // Tạo detached thread bằng pthread_detach()
    printf("2. Tạo thread và detach\n");
    id3 = malloc(sizeof(int));
    *id3 = 3;
    if (pthread_create(&thread2, NULL, detached_thread, id3) != 0) {
        perror("pthread_create failed");
        exit(EXIT_FAILURE);
    }
    pthread_detach(thread2);
    
    // Tạo detached thread bằng attribute
    printf("3. Tạo detached thread với attribute\n\n");
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    if (pthread_create(&thread3, &attr, worker_thread, "Backup Database") != 0) {
        perror("pthread_create failed");
        exit(EXIT_FAILURE);
    }
    
    pthread_attr_destroy(&attr);
    
    // Join joinable thread
    printf("Main: Đợi joinable thread...\n");
    pthread_join(thread1, NULL);
    printf("Main: Joinable thread đã kết thúc\n\n");
    
    // Không thể join detached threads
    // pthread_join(thread2, NULL); // Lỗi!
    // pthread_join(thread3, NULL); // Lỗi!
    
    printf("Main: Chờ detached threads tự kết thúc...\n");
    sleep(4);
    
    printf("\nMain: Kết thúc chương trình\n");
    printf("\nLưu ý:\n");
    printf("- Joinable thread: Cần pthread_join() để cleanup\n");
    printf("- Detached thread: Tự động cleanup khi kết thúc\n");
    printf("- Detached thread phù hợp cho background tasks\n");
    
    return 0;
}
