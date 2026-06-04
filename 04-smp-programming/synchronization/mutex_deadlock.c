/*
 * mutex_deadlock.c - Phát hiện và tránh deadlock
 * 
 * Demo deadlock và các cách giải quyết
 * 
 * Biên dịch: gcc -o mutex_deadlock mutex_deadlock.c -pthread
 * Chạy: ./mutex_deadlock
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

int account1_balance = 1000;
int account2_balance = 2000;

// Gây deadlock: Lock theo thứ tự khác nhau
void *deadlock_thread1(void *arg) {
    printf("Thread 1: Cố gắng lock mutex1...\n");
    pthread_mutex_lock(&mutex1);
    printf("Thread 1: Đã lock mutex1\n");
    
    sleep(1);  // Tăng khả năng deadlock
    
    printf("Thread 1: Cố gắng lock mutex2...\n");
    pthread_mutex_lock(&mutex2);
    printf("Thread 1: Đã lock mutex2\n");
    
    // Critical section
    account1_balance += 100;
    account2_balance -= 100;
    
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    
    printf("Thread 1: Hoàn thành\n");
    return NULL;
}

void *deadlock_thread2(void *arg) {
    printf("Thread 2: Cố gắng lock mutex2...\n");
    pthread_mutex_lock(&mutex2);
    printf("Thread 2: Đã lock mutex2\n");
    
    sleep(1);  // Tăng khả năng deadlock
    
    printf("Thread 2: Cố gắng lock mutex1...\n");
    pthread_mutex_lock(&mutex1);
    printf("Thread 2: Đã lock mutex1\n");
    
    // Critical section
    account2_balance += 50;
    account1_balance -= 50;
    
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    
    printf("Thread 2: Hoàn thành\n");
    return NULL;
}

// Giải pháp 1: Lock theo thứ tự cố định
void *fixed_order_thread1(void *arg) {
    printf("Thread 1: Lock mutex1, mutex2 (thứ tự cố định)\n");
    
    pthread_mutex_lock(&mutex1);
    pthread_mutex_lock(&mutex2);
    
    account1_balance += 100;
    account2_balance -= 100;
    printf("Thread 1: Chuyển 100 từ account2 -> account1\n");
    
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    
    printf("Thread 1: Hoàn thành\n");
    return NULL;
}

void *fixed_order_thread2(void *arg) {
    printf("Thread 2: Lock mutex1, mutex2 (thứ tự cố định)\n");
    
    pthread_mutex_lock(&mutex1);
    pthread_mutex_lock(&mutex2);
    
    account2_balance += 50;
    account1_balance -= 50;
    printf("Thread 2: Chuyển 50 từ account1 -> account2\n");
    
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    
    printf("Thread 2: Hoàn thành\n");
    return NULL;
}

// Giải pháp 2: Dùng pthread_mutex_trylock()
void *trylock_thread1(void *arg) {
    int retries = 0;
    
    while (1) {
        printf("Thread 1: Thử lock mutex1...\n");
        pthread_mutex_lock(&mutex1);
        
        printf("Thread 1: Thử lock mutex2...\n");
        if (pthread_mutex_trylock(&mutex2) == 0) {
            // Đã lock được cả hai
            account1_balance += 100;
            account2_balance -= 100;
            printf("Thread 1: Thành công chuyển tiền\n");
            
            pthread_mutex_unlock(&mutex2);
            pthread_mutex_unlock(&mutex1);
            break;
        } else {
            // Không lock được mutex2, thả mutex1 và thử lại
            pthread_mutex_unlock(&mutex1);
            retries++;
            printf("Thread 1: Không lock được mutex2, thử lại (lần %d)\n", retries);
            usleep(100000);  // Chờ một chút
        }
    }
    
    printf("Thread 1: Hoàn thành sau %d lần thử\n", retries + 1);
    return NULL;
}

void *trylock_thread2(void *arg) {
    int retries = 0;
    
    while (1) {
        printf("Thread 2: Thử lock mutex2...\n");
        pthread_mutex_lock(&mutex2);
        
        printf("Thread 2: Thử lock mutex1...\n");
        if (pthread_mutex_trylock(&mutex1) == 0) {
            // Đã lock được cả hai
            account2_balance += 50;
            account1_balance -= 50;
            printf("Thread 2: Thành công chuyển tiền\n");
            
            pthread_mutex_unlock(&mutex1);
            pthread_mutex_unlock(&mutex2);
            break;
        } else {
            // Không lock được mutex1, thả mutex2 và thử lại
            pthread_mutex_unlock(&mutex2);
            retries++;
            printf("Thread 2: Không lock được mutex1, thử lại (lần %d)\n", retries);
            usleep(100000);  // Chờ một chút
        }
    }
    
    printf("Thread 2: Hoàn thành sau %d lần thử\n", retries + 1);
    return NULL;
}

void demo_deadlock() {
    pthread_t t1, t2;
    
    printf("\n=== Demo DEADLOCK (Ctrl+C để dừng nếu bị treo) ===\n");
    printf("Hai threads lock theo thứ tự khác nhau -> DEADLOCK\n\n");
    
    account1_balance = 1000;
    account2_balance = 2000;
    
    pthread_create(&t1, NULL, deadlock_thread1, NULL);
    pthread_create(&t2, NULL, deadlock_thread2, NULL);
    
    // Đợi 5 giây, nếu chưa xong thì chắc chắn deadlock
    sleep(5);
    printf("\n!!! DEADLOCK xảy ra !!! (threads bị treo)\n");
    
    // Hủy threads (không khuyến khích trong production)
    pthread_cancel(t1);
    pthread_cancel(t2);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
}

void demo_fixed_order() {
    pthread_t t1, t2;
    
    printf("\n=== Giải pháp 1: Lock theo thứ tự cố định ===\n");
    printf("Tất cả threads lock theo cùng thứ tự -> KHÔNG DEADLOCK\n\n");
    
    account1_balance = 1000;
    account2_balance = 2000;
    
    pthread_create(&t1, NULL, fixed_order_thread1, NULL);
    pthread_create(&t2, NULL, fixed_order_thread2, NULL);
    
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    
    printf("\nSố dư cuối: Account1=%d, Account2=%d\n", 
           account1_balance, account2_balance);
}

void demo_trylock() {
    pthread_t t1, t2;
    
    printf("\n=== Giải pháp 2: Dùng trylock ===\n");
    printf("Nếu không lock được, thả lock và thử lại\n\n");
    
    account1_balance = 1000;
    account2_balance = 2000;
    
    pthread_create(&t1, NULL, trylock_thread1, NULL);
    pthread_create(&t2, NULL, trylock_thread2, NULL);
    
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    
    printf("\nSố dư cuối: Account1=%d, Account2=%d\n", 
           account1_balance, account2_balance);
}

int main() {
    printf("=== Demo Deadlock và Giải pháp ===\n");
    
    // Uncomment dòng dưới để demo deadlock (cẩn thận!)
    // demo_deadlock();
    
    demo_fixed_order();
    
    sleep(1);
    
    demo_trylock();
    
    pthread_mutex_destroy(&mutex1);
    pthread_mutex_destroy(&mutex2);
    
    printf("\n=== Các cách tránh Deadlock ===\n");
    printf("1. Lock order: Luôn lock theo thứ tự cố định\n");
    printf("2. Trylock: Thử lock, nếu không được thì thả và thử lại\n");
    printf("3. Timeout: Dùng pthread_mutex_timedlock()\n");
    printf("4. Deadlock detection: Tools như Valgrind helgrind\n");
    
    return 0;
}
