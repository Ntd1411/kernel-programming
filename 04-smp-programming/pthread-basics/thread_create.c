/*
 * thread_create.c - Tạo và kết thúc thread cơ bản
 * 
 * Biên dịch: gcc -o thread_create thread_create.c -pthread
 * Chạy: ./thread_create
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

void *thread_function(void *arg) {
    int thread_num = *(int *)arg;
    
    printf("Thread %d: Bắt đầu thực thi\n", thread_num);
    printf("Thread %d: Thread ID = %lu\n", thread_num, pthread_self());
    
    // Giả lập công việc
    sleep(2);
    
    printf("Thread %d: Kết thúc\n", thread_num);
    
    return NULL;
}

int main() {
    pthread_t threads[5];
    int thread_args[5];
    int i;
    
    printf("Main: Process ID = %d\n", getpid());
    printf("Main: Tạo %d threads...\n\n", 5);
    
    // Tạo threads
    for (i = 0; i < 5; i++) {
        thread_args[i] = i + 1;
        
        if (pthread_create(&threads[i], NULL, thread_function, &thread_args[i]) != 0) {
            perror("pthread_create failed");
            exit(EXIT_FAILURE);
        }
        
        printf("Main: Đã tạo thread %d\n", i + 1);
    }
    
    printf("\nMain: Chờ tất cả threads kết thúc...\n\n");
    
    // Đợi tất cả threads kết thúc
    for (i = 0; i < 5; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join failed");
            exit(EXIT_FAILURE);
        }
        printf("Main: Thread %d đã kết thúc\n", i + 1);
    }
    
    printf("\nMain: Tất cả threads đã hoàn thành\n");
    
    return 0;
}
