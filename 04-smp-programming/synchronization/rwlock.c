/*
 * rwlock.c - Reader-Writer locks
 * 
 * Nhiều readers có thể đọc cùng lúc
 * Chỉ 1 writer có thể ghi
 * 
 * Biên dịch: gcc -o rwlock rwlock.c -pthread
 * Chạy: ./rwlock
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_READERS 5
#define NUM_WRITERS 2

pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

int shared_data = 0;
int read_count = 0;
int write_count = 0;

void *reader_thread(void *arg) {
    int id = *(int *)arg;
    int i;
    
    for (i = 0; i < 3; i++) {
        printf("Reader %d: Cố gắng đọc...\n", id);
        
        pthread_rwlock_rdlock(&rwlock);
        
        // Đọc dữ liệu (nhiều readers có thể đọc cùng lúc)
        printf("Reader %d: Đọc shared_data = %d\n", id, shared_data);
        read_count++;
        
        sleep(1);  // Giả lập đọc dữ liệu
        
        pthread_rwlock_unlock(&rwlock);
        
        printf("Reader %d: Đã unlock\n", id);
        
        sleep(rand() % 2);  // Nghỉ trước lần đọc tiếp
    }
    
    printf("Reader %d: Hoàn thành\n", id);
    return NULL;
}

void *writer_thread(void *arg) {
    int id = *(int *)arg;
    int i;
    
    for (i = 0; i < 3; i++) {
        printf("Writer %d: Cố gắng ghi...\n", id);
        
        pthread_rwlock_wrlock(&rwlock);
        
        // Ghi dữ liệu (chỉ 1 writer, block tất cả readers)
        shared_data++;
        write_count++;
        printf("Writer %d: GHI shared_data = %d\n", id, shared_data);
        
        sleep(2);  // Giả lập ghi dữ liệu (lâu hơn đọc)
        
        pthread_rwlock_unlock(&rwlock);
        
        printf("Writer %d: Đã unlock\n", id);
        
        sleep(rand() % 2);  // Nghỉ trước lần ghi tiếp
    }
    
    printf("Writer %d: Hoàn thành\n", id);
    return NULL;
}

void *priority_reader(void *arg) {
    int id = *(int *)arg;
    
    printf("Priority Reader %d: Thử đọc với trylock...\n", id);
    
    if (pthread_rwlock_tryrdlock(&rwlock) == 0) {
        printf("Priority Reader %d: Đọc thành công = %d\n", id, shared_data);
        sleep(1);
        pthread_rwlock_unlock(&rwlock);
    } else {
        printf("Priority Reader %d: Không thể lock (có writer đang ghi)\n", id);
    }
    
    return NULL;
}

int main() {
    pthread_t readers[NUM_READERS];
    pthread_t writers[NUM_WRITERS];
    int reader_ids[NUM_READERS];
    int writer_ids[NUM_WRITERS];
    int i;
    
    srand(time(NULL));
    
    printf("=== Demo Reader-Writer Lock ===\n");
    printf("Tạo %d readers và %d writers\n\n", NUM_READERS, NUM_WRITERS);
    
    // Tạo writer threads trước
    for (i = 0; i < NUM_WRITERS; i++) {
        writer_ids[i] = i + 1;
        if (pthread_create(&writers[i], NULL, writer_thread, &writer_ids[i]) != 0) {
            perror("pthread_create writer failed");
            exit(EXIT_FAILURE);
        }
    }
    
    sleep(1);  // Cho writer chạy trước
    
    // Tạo reader threads
    for (i = 0; i < NUM_READERS; i++) {
        reader_ids[i] = i + 1;
        if (pthread_create(&readers[i], NULL, reader_thread, &reader_ids[i]) != 0) {
            perror("pthread_create reader failed");
            exit(EXIT_FAILURE);
        }
    }
    
    // Đợi tất cả threads
    for (i = 0; i < NUM_WRITERS; i++) {
        pthread_join(writers[i], NULL);
    }
    
    for (i = 0; i < NUM_READERS; i++) {
        pthread_join(readers[i], NULL);
    }
    
    printf("\n=== Kết quả ===\n");
    printf("Giá trị cuối: %d\n", shared_data);
    printf("Số lần đọc: %d\n", read_count);
    printf("Số lần ghi: %d\n", write_count);
    
    // Test trylock
    printf("\n=== Demo tryrdlock ===\n");
    pthread_t try_reader;
    int try_id = 99;
    
    pthread_rwlock_wrlock(&rwlock);  // Lock để test trylock fail
    printf("Main: Đã wrlock, giờ test tryrdlock...\n");
    
    pthread_create(&try_reader, NULL, priority_reader, &try_id);
    pthread_join(try_reader, NULL);
    
    pthread_rwlock_unlock(&rwlock);
    
    // Cleanup
    pthread_rwlock_destroy(&rwlock);
    
    printf("\n=== Đặc điểm RWLock ===\n");
    printf("✓ Nhiều readers có thể đọc cùng lúc\n");
    printf("✓ Chỉ 1 writer có thể ghi (exclusive)\n");
    printf("✓ Writer block tất cả readers và writers khác\n");
    printf("✓ Phù hợp cho read-heavy workloads\n");
    
    return 0;
}
