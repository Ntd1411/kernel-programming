/*
 * ipc_basics.c - Inter-Process Communication cơ bản
 * 
 * Các phương thức IPC:
 * - Unnamed pipes (pipe)
 * - Named pipes (FIFO)
 * - Message queues
 * - Shared memory
 * - Semaphores
 * 
 * Biên dịch: gcc -o ipc_basics ipc_basics.c -lrt -pthread
 * Chạy: ./ipc_basics
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#define FIFO_NAME "/tmp/my_fifo"
#define SHM_SIZE 1024

void print_separator() {
    printf("================================================\n");
}

// ============================================
// UNNAMED PIPES
// ============================================

void example1_unnamed_pipe() {
    printf("\n=== Ví dụ 1: Unnamed Pipe (pipe) ===\n");
    printf("Pipe dùng để giao tiếp giữa cha và con\n\n");
    
    int pipefd[2];
    pid_t pid;
    char write_msg[] = "Xin chào từ tiến trình cha!";
    char read_msg[100];
    
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return;
    }
    
    printf("Đã tạo pipe: pipefd[0]=%d (đọc), pipefd[1]=%d (ghi)\n", 
           pipefd[0], pipefd[1]);
    
    pid = fork();
    
    if (pid < 0) {
        perror("fork");
        return;
    } else if (pid == 0) {
        // Tiến trình con - đọc từ pipe
        close(pipefd[1]);  // Đóng đầu ghi
        
        printf("[CON] Đang chờ dữ liệu từ pipe...\n");
        ssize_t n = read(pipefd[0], read_msg, sizeof(read_msg));
        
        if (n > 0) {
            read_msg[n] = '\0';
            printf("[CON] Nhận được: '%s'\n", read_msg);
        }
        
        close(pipefd[0]);
        exit(0);
    } else {
        // Tiến trình cha - ghi vào pipe
        close(pipefd[0]);  // Đóng đầu đọc
        
        printf("[CHA] Gửi dữ liệu qua pipe...\n");
        write(pipefd[1], write_msg, strlen(write_msg));
        
        close(pipefd[1]);
        wait(NULL);
        printf("[CHA] Hoàn thành!\n");
    }
}

void example2_pipe_bidirectional() {
    printf("\n=== Ví dụ 2: Two-way Communication với 2 Pipes ===\n");
    
    int pipe1[2], pipe2[2];  // pipe1: cha->con, pipe2: con->cha
    pid_t pid;
    char msg_to_child[] = "Ping từ cha";
    char msg_to_parent[] = "Pong từ con";
    char buffer[100];
    
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("pipe");
        return;
    }
    
    pid = fork();
    
    if (pid < 0) {
        perror("fork");
        return;
    } else if (pid == 0) {
        // Con
        close(pipe1[1]);  // Đóng pipe1 ghi
        close(pipe2[0]);  // Đóng pipe2 đọc
        
        // Đọc từ cha
        ssize_t n = read(pipe1[0], buffer, sizeof(buffer));
        if (n > 0) {
            buffer[n] = '\0';
            printf("[CON] Nhận: '%s'\n", buffer);
        }
        
        // Gửi về cha
        printf("[CON] Gửi phản hồi...\n");
        write(pipe2[1], msg_to_parent, strlen(msg_to_parent));
        
        close(pipe1[0]);
        close(pipe2[1]);
        exit(0);
    } else {
        // Cha
        close(pipe1[0]);  // Đóng pipe1 đọc
        close(pipe2[1]);  // Đóng pipe2 ghi
        
        // Gửi đến con
        printf("[CHA] Gửi tin nhắn đến con...\n");
        write(pipe1[1], msg_to_child, strlen(msg_to_child));
        
        // Đọc phản hồi từ con
        ssize_t n = read(pipe2[0], buffer, sizeof(buffer));
        if (n > 0) {
            buffer[n] = '\0';
            printf("[CHA] Nhận phản hồi: '%s'\n", buffer);
        }
        
        close(pipe1[1]);
        close(pipe2[0]);
        wait(NULL);
        printf("[CHA] Hoàn thành!\n");
    }
}

// ============================================
// NAMED PIPES (FIFO)
// ============================================

void example3_named_pipe_writer() {
    printf("\n=== Ví dụ 3: Named Pipe (FIFO) - Writer ===\n");
    
    // Tạo FIFO
    unlink(FIFO_NAME);  // Xóa nếu đã tồn tại
    
    if (mkfifo(FIFO_NAME, 0666) == -1) {
        if (errno != EEXIST) {
            perror("mkfifo");
            return;
        }
    }
    
    printf("Đã tạo FIFO: %s\n", FIFO_NAME);
    printf("Mở FIFO để ghi (sẽ block cho đến khi có reader)...\n");
    
    int fd = open(FIFO_NAME, O_WRONLY);
    if (fd == -1) {
        perror("open");
        return;
    }
    
    printf("FIFO đã được mở, gửi dữ liệu...\n");
    
    for (int i = 1; i <= 3; i++) {
        char msg[100];
        snprintf(msg, sizeof(msg), "Tin nhắn số %d từ writer", i);
        
        write(fd, msg, strlen(msg) + 1);
        printf("Đã gửi: %s\n", msg);
        sleep(1);
    }
    
    close(fd);
    printf("Đóng FIFO\n");
}

void example3_named_pipe_reader() {
    printf("\n=== Ví dụ 3: Named Pipe (FIFO) - Reader ===\n");
    
    printf("Mở FIFO để đọc...\n");
    
    int fd = open(FIFO_NAME, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return;
    }
    
    printf("FIFO đã được mở, chờ nhận dữ liệu...\n");
    
    char buffer[100];
    ssize_t n;
    
    while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
        printf("Nhận được: %s\n", buffer);
    }
    
    close(fd);
    unlink(FIFO_NAME);
    printf("Đóng FIFO và xóa file\n");
}

void example3_named_pipe_demo() {
    printf("\n=== Ví dụ 3: Named Pipe Demo ===\n");
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        return;
    } else if (pid == 0) {
        // Con - reader
        sleep(1);  // Đợi cha tạo FIFO
        example3_named_pipe_reader();
        exit(0);
    } else {
        // Cha - writer
        example3_named_pipe_writer();
        wait(NULL);
    }
}

// ============================================
// MESSAGE QUEUES
// ============================================

struct msg_buffer {
    long msg_type;
    char msg_text[100];
};

void example4_message_queue() {
    printf("\n=== Ví dụ 4: Message Queue ===\n");
    
    key_t key = ftok(".", 'a');
    if (key == -1) {
        perror("ftok");
        return;
    }
    
    int msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget");
        return;
    }
    
    printf("Đã tạo message queue với ID: %d\n", msgid);
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        return;
    } else if (pid == 0) {
        // Con - nhận tin nhắn
        printf("[CON] Chờ nhận tin nhắn...\n");
        
        struct msg_buffer message;
        
        for (int i = 0; i < 3; i++) {
            if (msgrcv(msgid, &message, sizeof(message.msg_text), 1, 0) == -1) {
                perror("msgrcv");
                exit(1);
            }
            printf("[CON] Nhận được (type=%ld): %s\n", 
                   message.msg_type, message.msg_text);
        }
        
        exit(0);
    } else {
        // Cha - gửi tin nhắn
        sleep(1);
        
        struct msg_buffer message;
        message.msg_type = 1;
        
        for (int i = 1; i <= 3; i++) {
            snprintf(message.msg_text, sizeof(message.msg_text), 
                     "Tin nhắn số %d từ cha", i);
            
            printf("[CHA] Gửi: %s\n", message.msg_text);
            
            if (msgsnd(msgid, &message, sizeof(message.msg_text), 0) == -1) {
                perror("msgsnd");
                break;
            }
            
            sleep(1);
        }
        
        wait(NULL);
        
        // Xóa message queue
        if (msgctl(msgid, IPC_RMID, NULL) == -1) {
            perror("msgctl");
        } else {
            printf("[CHA] Đã xóa message queue\n");
        }
    }
}

void example5_message_priority() {
    printf("\n=== Ví dụ 5: Message Queue với Priority ===\n");
    
    key_t key = ftok(".", 'b');
    if (key == -1) {
        perror("ftok");
        return;
    }
    
    int msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget");
        return;
    }
    
    printf("Tạo message queue cho demo priority\n");
    
    // Gửi các tin nhắn với priority khác nhau
    struct msg_buffer message;
    
    printf("\nGửi tin nhắn với các mức ưu tiên:\n");
    
    // Priority thấp (type = 1)
    message.msg_type = 1;
    strcpy(message.msg_text, "Priority thấp (type=1)");
    msgsnd(msgid, &message, sizeof(message.msg_text), 0);
    printf("Đã gửi: %s\n", message.msg_text);
    
    // Priority cao (type = 3)
    message.msg_type = 3;
    strcpy(message.msg_text, "Priority cao (type=3)");
    msgsnd(msgid, &message, sizeof(message.msg_text), 0);
    printf("Đã gửi: %s\n", message.msg_text);
    
    // Priority trung bình (type = 2)
    message.msg_type = 2;
    strcpy(message.msg_text, "Priority trung bình (type=2)");
    msgsnd(msgid, &message, sizeof(message.msg_text), 0);
    printf("Đã gửi: %s\n", message.msg_text);
    
    // Nhận theo thứ tự priority (type cao trước)
    printf("\nNhận tin nhắn theo thứ tự priority:\n");
    
    for (int type = 3; type >= 1; type--) {
        if (msgrcv(msgid, &message, sizeof(message.msg_text), type, 0) != -1) {
            printf("Nhận (type=%ld): %s\n", message.msg_type, message.msg_text);
        }
    }
    
    msgctl(msgid, IPC_RMID, NULL);
    printf("\nĐã xóa message queue\n");
}

// ============================================
// SHARED MEMORY
// ============================================

void example6_shared_memory() {
    printf("\n=== Ví dụ 6: Shared Memory ===\n");
    
    key_t key = ftok(".", 'c');
    if (key == -1) {
        perror("ftok");
        return;
    }
    
    int shmid = shmget(key, SHM_SIZE, 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        return;
    }
    
    printf("Đã tạo shared memory segment với ID: %d\n", shmid);
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        return;
    } else if (pid == 0) {
        // Con - đọc từ shared memory
        sleep(1);
        
        char *shm_ptr = (char *)shmat(shmid, NULL, 0);
        if (shm_ptr == (char *)-1) {
            perror("shmat");
            exit(1);
        }
        
        printf("[CON] Đọc từ shared memory: %s\n", shm_ptr);
        
        // Ghi phản hồi
        strcpy(shm_ptr, "Đã nhận được tin nhắn!");
        
        shmdt(shm_ptr);
        exit(0);
    } else {
        // Cha - ghi vào shared memory
        char *shm_ptr = (char *)shmat(shmid, NULL, 0);
        if (shm_ptr == (char *)-1) {
            perror("shmat");
            return;
        }
        
        printf("[CHA] Ghi vào shared memory...\n");
        strcpy(shm_ptr, "Xin chào từ tiến trình cha!");
        
        wait(NULL);
        
        printf("[CHA] Đọc phản hồi: %s\n", shm_ptr);
        
        shmdt(shm_ptr);
        
        // Xóa shared memory
        if (shmctl(shmid, IPC_RMID, NULL) == -1) {
            perror("shmctl");
        } else {
            printf("[CHA] Đã xóa shared memory\n");
        }
    }
}

void example7_shared_memory_posix() {
    printf("\n=== Ví dụ 7: POSIX Shared Memory ===\n");
    
    const char *shm_name = "/my_shm";
    
    // Xóa nếu đã tồn tại
    shm_unlink(shm_name);
    
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return;
    }
    
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        close(shm_fd);
        return;
    }
    
    printf("Đã tạo POSIX shared memory: %s\n", shm_name);
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        return;
    } else if (pid == 0) {
        // Con
        sleep(1);
        
        void *ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, 
                        MAP_SHARED, shm_fd, 0);
        if (ptr == MAP_FAILED) {
            perror("mmap");
            exit(1);
        }
        
        printf("[CON] Đọc: %s\n", (char *)ptr);
        
        munmap(ptr, SHM_SIZE);
        close(shm_fd);
        exit(0);
    } else {
        // Cha
        void *ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, 
                        MAP_SHARED, shm_fd, 0);
        if (ptr == MAP_FAILED) {
            perror("mmap");
            return;
        }
        
        printf("[CHA] Ghi dữ liệu...\n");
        strcpy((char *)ptr, "Dữ liệu từ POSIX shared memory!");
        
        wait(NULL);
        
        munmap(ptr, SHM_SIZE);
        close(shm_fd);
        shm_unlink(shm_name);
        printf("[CHA] Đã xóa shared memory\n");
    }
}

// ============================================
// SEMAPHORES
// ============================================

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

void example8_system_v_semaphore() {
    printf("\n=== Ví dụ 8: System V Semaphore ===\n");
    
    key_t key = ftok(".", 'd');
    if (key == -1) {
        perror("ftok");
        return;
    }
    
    int semid = semget(key, 1, 0666 | IPC_CREAT);
    if (semid == -1) {
        perror("semget");
        return;
    }
    
    // Khởi tạo semaphore = 1
    union semun arg;
    arg.val = 1;
    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("semctl");
        return;
    }
    
    printf("Đã tạo semaphore với ID: %d (giá trị ban đầu = 1)\n", semid);
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        return;
    } else if (pid == 0) {
        // Con
        struct sembuf sb;
        
        // Wait (P operation)
        printf("[CON] Chờ semaphore...\n");
        sb.sem_num = 0;
        sb.sem_op = -1;
        sb.sem_flg = 0;
        
        if (semop(semid, &sb, 1) == -1) {
            perror("semop");
            exit(1);
        }
        
        printf("[CON] Đã lấy semaphore, vào critical section\n");
        printf("[CON] Làm việc trong 2 giây...\n");
        sleep(2);
        
        // Signal (V operation)
        sb.sem_op = 1;
        if (semop(semid, &sb, 1) == -1) {
            perror("semop");
            exit(1);
        }
        
        printf("[CON] Đã thả semaphore\n");
        exit(0);
    } else {
        // Cha
        sleep(1);
        
        struct sembuf sb;
        
        printf("[CHA] Chờ semaphore...\n");
        sb.sem_num = 0;
        sb.sem_op = -1;
        sb.sem_flg = 0;
        
        if (semop(semid, &sb, 1) == -1) {
            perror("semop");
            return;
        }
        
        printf("[CHA] Đã lấy semaphore, vào critical section\n");
        printf("[CHA] Làm việc trong 1 giây...\n");
        sleep(1);
        
        sb.sem_op = 1;
        if (semop(semid, &sb, 1) == -1) {
            perror("semop");
            return;
        }
        
        printf("[CHA] Đã thả semaphore\n");
        
        wait(NULL);
        
        // Xóa semaphore
        if (semctl(semid, 0, IPC_RMID) == -1) {
            perror("semctl");
        } else {
            printf("[CHA] Đã xóa semaphore\n");
        }
    }
}

void example9_posix_semaphore() {
    printf("\n=== Ví dụ 9: POSIX Named Semaphore ===\n");
    
    const char *sem_name = "/my_sem";
    
    // Xóa nếu đã tồn tại
    sem_unlink(sem_name);
    
    sem_t *sem = sem_open(sem_name, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        return;
    }
    
    printf("Đã tạo POSIX semaphore: %s\n", sem_name);
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        return;
    } else if (pid == 0) {
        // Con
        sem_t *child_sem = sem_open(sem_name, 0);
        if (child_sem == SEM_FAILED) {
            perror("sem_open");
            exit(1);
        }
        
        printf("[CON] Chờ semaphore...\n");
        sem_wait(child_sem);
        
        printf("[CON] Đã lấy semaphore, vào critical section\n");
        printf("[CON] Làm việc trong 2 giây...\n");
        sleep(2);
        
        sem_post(child_sem);
        printf("[CON] Đã thả semaphore\n");
        
        sem_close(child_sem);
        exit(0);
    } else {
        // Cha
        sleep(1);
        
        printf("[CHA] Chờ semaphore...\n");
        sem_wait(sem);
        
        printf("[CHA] Đã lấy semaphore, vào critical section\n");
        printf("[CHA] Làm việc trong 1 giây...\n");
        sleep(1);
        
        sem_post(sem);
        printf("[CHA] Đã thả semaphore\n");
        
        wait(NULL);
        
        sem_close(sem);
        sem_unlink(sem_name);
        printf("[CHA] Đã xóa semaphore\n");
    }
}

// ============================================
// MAIN MENU
// ============================================

void print_menu() {
    printf("\n");
    print_separator();
    printf("       INTER-PROCESS COMMUNICATION DEMO\n");
    print_separator();
    printf("\n--- PIPES ---\n");
    printf("1. Unnamed Pipe (pipe)\n");
    printf("2. Two-way Communication với 2 Pipes\n");
    printf("3. Named Pipe (FIFO)\n");
    printf("\n--- MESSAGE QUEUES ---\n");
    printf("4. Message Queue cơ bản\n");
    printf("5. Message Queue với Priority\n");
    printf("\n--- SHARED MEMORY ---\n");
    printf("6. System V Shared Memory\n");
    printf("7. POSIX Shared Memory\n");
    printf("\n--- SEMAPHORES ---\n");
    printf("8. System V Semaphore\n");
    printf("9. POSIX Named Semaphore\n");
    printf("\n0. Thoát\n");
    print_separator();
    printf("Chọn: ");
}

int main() {
    printf("=== IPC (INTER-PROCESS COMMUNICATION) EXAMPLES ===\n");
    printf("PID: %d\n", getpid());
    
    example1_unnamed_pipe();
    sleep(1);
    
    example2_pipe_bidirectional();
    sleep(1);
    
    example3_named_pipe_demo();
    sleep(1);
    
    example4_message_queue();
    sleep(1);
    
    example5_message_priority();
    sleep(1);
    
    example6_shared_memory();
    sleep(1);
    
    example7_shared_memory_posix();
    sleep(1);
    
    example8_system_v_semaphore();
    sleep(1);
    
    example9_posix_semaphore();
    
    printf("\n=== HOÀN THÀNH ===\n");
    printf("\nTóm tắt IPC methods:\n");
    printf("  pipe()        - unnamed pipe cho cha-con\n");
    printf("  mkfifo()      - named pipe (FIFO)\n");
    printf("  msgget()      - System V message queue\n");
    printf("  shmget()      - System V shared memory\n");
    printf("  shm_open()    - POSIX shared memory\n");
    printf("  semget()      - System V semaphore\n");
    printf("  sem_open()    - POSIX named semaphore\n");
    
    return 0;
}
