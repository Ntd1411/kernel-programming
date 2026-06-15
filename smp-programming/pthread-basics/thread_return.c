/*
 * thread_return.c - Nhận giá trị trả về từ thread
 * 
 * Biên dịch: gcc -o thread_return thread_return.c -pthread
 * Chạy: ./thread_return
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

typedef struct {
    int sum;
    int count;
    double average;
} result_t;

void *calculate_sum(void *arg) {
    int *numbers = (int *)arg;
    result_t *result = malloc(sizeof(result_t));
    int i;
    
    if (result == NULL) {
        return NULL;
    }
    
    result->sum = 0;
    result->count = 10;
    
    printf("Thread: Tính tổng của 10 số...\n");
    
    for (i = 0; i < 10; i++) {
        result->sum += numbers[i];
        printf("Thread: numbers[%d] = %d, sum = %d\n", i, numbers[i], result->sum);
        usleep(100000);
    }
    
    result->average = (double)result->sum / result->count;
    
    printf("Thread: Hoàn thành tính toán\n");
    
    return (void *)result;
}

void *factorial(void *arg) {
    int n = *(int *)arg;
    long long *result = malloc(sizeof(long long));
    int i;
    
    if (result == NULL) {
        return NULL;
    }
    
    *result = 1;
    
    printf("Thread: Tính giai thừa của %d...\n", n);
    
    for (i = 1; i <= n; i++) {
        *result *= i;
        printf("Thread: %d! = %lld\n", i, *result);
        usleep(200000);
    }
    
    return (void *)result;
}

int main() {
    pthread_t thread1, thread2;
    void *ret_val1, *ret_val2;
    int numbers[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    int n = 10;
    
    printf("Main: Tạo thread 1 để tính tổng\n");
    if (pthread_create(&thread1, NULL, calculate_sum, numbers) != 0) {
        perror("pthread_create failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Main: Tạo thread 2 để tính giai thừa\n\n");
    if (pthread_create(&thread2, NULL, factorial, &n) != 0) {
        perror("pthread_create failed");
        exit(EXIT_FAILURE);
    }
    
    // Đợi thread 1 và lấy kết quả
    printf("\nMain: Chờ thread 1 hoàn thành...\n");
    if (pthread_join(thread1, &ret_val1) != 0) {
        perror("pthread_join failed");
        exit(EXIT_FAILURE);
    }
    
    if (ret_val1 != NULL) {
        result_t *result = (result_t *)ret_val1;
        printf("Main: Kết quả từ thread 1:\n");
        printf("  - Tổng: %d\n", result->sum);
        printf("  - Số lượng: %d\n", result->count);
        printf("  - Trung bình: %.2f\n", result->average);
        free(result);
    }
    
    // Đợi thread 2 và lấy kết quả
    printf("\nMain: Chờ thread 2 hoàn thành...\n");
    if (pthread_join(thread2, &ret_val2) != 0) {
        perror("pthread_join failed");
        exit(EXIT_FAILURE);
    }
    
    if (ret_val2 != NULL) {
        long long *result = (long long *)ret_val2;
        printf("Main: Kết quả từ thread 2:\n");
        printf("  - %d! = %lld\n", n, *result);
        free(result);
    }
    
    printf("\nMain: Tất cả threads đã hoàn thành\n");
    
    return 0;
}
