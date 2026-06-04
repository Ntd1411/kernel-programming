/*
 * thread_args.c - Truyền tham số cho thread
 * 
 * Biên dịch: gcc -o thread_args thread_args.c -pthread
 * Chạy: ./thread_args
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

// Struct để truyền nhiều tham số
typedef struct {
    int id;
    char name[50];
    int iterations;
} thread_data_t;

void *thread_function(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    int i;
    
    printf("Thread %d (%s): Bắt đầu với %d iterations\n", 
           data->id, data->name, data->iterations);
    
    for (i = 0; i < data->iterations; i++) {
        printf("Thread %d: Iteration %d/%d\n", 
               data->id, i + 1, data->iterations);
        sleep(1);
    }
    
    printf("Thread %d: Hoàn thành\n", data->id);
    
    return NULL;
}

int main() {
    pthread_t threads[3];
    thread_data_t thread_data[3];
    int i;
    
    // Khởi tạo dữ liệu cho mỗi thread
    thread_data[0].id = 1;
    strcpy(thread_data[0].name, "Worker-A");
    thread_data[0].iterations = 3;
    
    thread_data[1].id = 2;
    strcpy(thread_data[1].name, "Worker-B");
    thread_data[1].iterations = 2;
    
    thread_data[2].id = 3;
    strcpy(thread_data[2].name, "Worker-C");
    thread_data[2].iterations = 4;
    
    printf("Main: Tạo %d threads với tham số khác nhau\n\n", 3);
    
    // Tạo threads với tham số
    for (i = 0; i < 3; i++) {
        if (pthread_create(&threads[i], NULL, thread_function, &thread_data[i]) != 0) {
            perror("pthread_create failed");
            exit(EXIT_FAILURE);
        }
    }
    
    // Đợi threads kết thúc
    for (i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\nMain: Tất cả threads đã hoàn thành\n");
    
    return 0;
}
