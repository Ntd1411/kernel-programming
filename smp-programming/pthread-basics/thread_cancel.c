/*
 * thread_cancel.c - Hủy thread
 * 
 * Biên dịch: gcc -o thread_cancel thread_cancel.c -pthread
 * Chạy: ./thread_cancel
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

void cleanup_handler(void *arg) {
    char *name = (char *)arg;
    printf("Cleanup: Giải phóng tài nguyên cho %s\n", name);
}

void *cancelable_thread(void *arg) {
    int oldstate, oldtype;
    
    // Đăng ký cleanup handler
    pthread_cleanup_push(cleanup_handler, "cancelable_thread");
    
    printf("Cancelable Thread: Bắt đầu\n");
    printf("Cancelable Thread: Cancellation enabled\n");
    
    // Cho phép cancel
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);
    
    int i;
    for (i = 0; i < 20; i++) {
        printf("Cancelable Thread: Đang làm việc... %d/20\n", i + 1);
        sleep(1);
        
        // Cancellation point
        pthread_testcancel();
    }
    
    printf("Cancelable Thread: Hoàn thành bình thường\n");
    
    pthread_cleanup_pop(1);
    
    return NULL;
}

void *non_cancelable_thread(void *arg) {
    int oldstate;
    
    pthread_cleanup_push(cleanup_handler, "non_cancelable_thread");
    
    printf("Non-cancelable Thread: Bắt đầu\n");
    
    // Tắt cancellation
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
    printf("Non-cancelable Thread: Cancellation disabled\n");
    
    int i;
    for (i = 0; i < 10; i++) {
        printf("Non-cancelable Thread: Critical work %d/10\n", i + 1);
        sleep(1);
    }
    
    // Bật lại cancellation
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
    printf("Non-cancelable Thread: Cancellation enabled lại\n");
    
    printf("Non-cancelable Thread: Hoàn thành\n");
    
    pthread_cleanup_pop(1);
    
    return NULL;
}

void *async_cancel_thread(void *arg) {
    int oldtype;
    
    pthread_cleanup_push(cleanup_handler, "async_cancel_thread");
    
    printf("Async Thread: Bắt đầu\n");
    
    // Asynchronous cancellation (nguy hiểm!)
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);
    printf("Async Thread: Async cancellation enabled\n");
    
    int i;
    for (i = 0; i < 1000000000; i++) {
        // Busy work - có thể bị cancel bất cứ lúc nào
    }
    
    printf("Async Thread: Hoàn thành\n");
    
    pthread_cleanup_pop(1);
    
    return NULL;
}

int main() {
    pthread_t thread1, thread2, thread3;
    void *ret_val;
    
    printf("=== Demo Thread Cancellation ===\n\n");
    
    // Test 1: Cancel thread bình thường
    printf("Test 1: Hủy cancelable thread\n");
    pthread_create(&thread1, NULL, cancelable_thread, NULL);
    
    sleep(5);
    printf("\nMain: Gửi cancel request đến thread 1\n");
    pthread_cancel(thread1);
    
    pthread_join(thread1, &ret_val);
    if (ret_val == PTHREAD_CANCELED) {
        printf("Main: Thread 1 đã bị cancel\n\n");
    }
    
    sleep(2);
    
    // Test 2: Thử cancel non-cancelable thread
    printf("Test 2: Thử hủy non-cancelable thread\n");
    pthread_create(&thread2, NULL, non_cancelable_thread, NULL);
    
    sleep(3);
    printf("\nMain: Gửi cancel request đến thread 2\n");
    pthread_cancel(thread2);
    
    pthread_join(thread2, &ret_val);
    if (ret_val == PTHREAD_CANCELED) {
        printf("Main: Thread 2 đã bị cancel\n");
    } else {
        printf("Main: Thread 2 hoàn thành bình thường\n\n");
    }
    
    sleep(2);
    
    // Test 3: Async cancellation
    printf("Test 3: Async cancellation\n");
    pthread_create(&thread3, NULL, async_cancel_thread, NULL);
    
    usleep(100000);
    printf("Main: Gửi cancel request đến async thread\n");
    pthread_cancel(thread3);
    
    pthread_join(thread3, &ret_val);
    if (ret_val == PTHREAD_CANCELED) {
        printf("Main: Async thread đã bị cancel\n");
    }
    
    printf("\n=== Kết thúc demo ===\n");
    printf("\nCancellation Types:\n");
    printf("- DEFERRED: Cancel tại cancellation points (an toàn)\n");
    printf("- ASYNCHRONOUS: Cancel ngay lập tức (nguy hiểm!)\n");
    printf("\nCancellation State:\n");
    printf("- ENABLE: Cho phép cancel\n");
    printf("- DISABLE: Không cho phép cancel (critical sections)\n");
    
    return 0;
}
