/*
 * exec_family.c - Các hàm exec để thực thi chương trình khác
 * 
 * exec family: execl, execlp, execle, execv, execvp, execvpe
 * 
 * Biên dịch: gcc -o exec_family exec_family.c
 * Chạy: ./exec_family
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

void example1_execl() {
    printf("\n=== Ví dụ 1: execl() ===\n");
    printf("Thực thi /bin/ls với đường dẫn đầy đủ\n\n");
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Con: thực thi ls
        printf("Thực thi: execl(\"/bin/ls\", \"ls\", \"-l\", \"/tmp\", NULL)\n\n");
        execl("/bin/ls", "ls", "-l", "/tmp", NULL);
        
        // Chỉ chạy đến đây nếu execl thất bại
        perror("execl");
        exit(EXIT_FAILURE);
    } else {
        // Cha đợi
        wait(NULL);
        printf("\nCon đã kết thúc\n");
    }
}

void example2_execlp() {
    printf("\n=== Ví dụ 2: execlp() ===\n");
    printf("Thực thi ls (tìm trong PATH)\n\n");
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Con: thực thi ls với PATH
        printf("Thực thi: execlp(\"ls\", \"ls\", \"-lh\", NULL)\n\n");
        execlp("ls", "ls", "-lh", NULL);
        
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        wait(NULL);
        printf("\nCon đã kết thúc\n");
    }
}

void example3_execv() {
    printf("\n=== Ví dụ 3: execv() ===\n");
    printf("Thực thi với array of arguments\n\n");
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Con: chuẩn bị arguments
        char *args[] = {
            "ls",
            "-l",
            "-a",
            "/home",
            NULL
        };
        
        printf("Thực thi: execv(\"/bin/ls\", [\"ls\", \"-l\", \"-a\", \"/home\", NULL])\n\n");
        execv("/bin/ls", args);
        
        perror("execv");
        exit(EXIT_FAILURE);
    } else {
        wait(NULL);
        printf("\nCon đã kết thúc\n");
    }
}

void example4_execvp() {
    printf("\n=== Ví dụ 4: execvp() ===\n");
    printf("Thực thi echo với array of arguments\n\n");
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        char *args[] = {
            "echo",
            "Hello",
            "from",
            "execvp!",
            NULL
        };
        
        printf("Thực thi: execvp(\"echo\", [\"echo\", \"Hello\", \"from\", \"execvp!\", NULL])\n\n");
        execvp("echo", args);
        
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        wait(NULL);
        printf("Con đã kết thúc\n");
    }
}

void example5_execle() {
    printf("\n=== Ví dụ 5: execle() với environment ===\n");
    printf("Thực thi với custom environment variables\n\n");
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Custom environment
        char *envp[] = {
            "MY_VAR=Hello",
            "PATH=/bin:/usr/bin",
            NULL
        };
        
        printf("Thực thi: execle(\"/bin/sh\", \"sh\", \"-c\", \"echo $MY_VAR\", NULL, envp)\n");
        printf("Environment: MY_VAR=Hello, PATH=/bin:/usr/bin\n\n");
        execle("/bin/sh", "sh", "-c", "echo $MY_VAR", NULL, envp);
        
        perror("execle");
        exit(EXIT_FAILURE);
    } else {
        wait(NULL);
        printf("Con đã kết thúc\n");
    }
}

void example6_exec_shell_script() {
    printf("\n=== Ví dụ 6: Thực thi shell command ===\n\n");
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Thực thi shell command
        printf("Thực thi: execlp(\"sh\", \"sh\", \"-c\", \"echo 'Current user:' $(whoami) && echo 'PWD:' $(pwd)\", NULL)\n\n");
        execlp("sh", "sh", "-c", 
               "echo 'Current user:' $(whoami) && echo 'PWD:' $(pwd)",
               NULL);
        
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        wait(NULL);
        printf("Con đã kết thúc\n");
    }
}

void example7_exec_with_input() {
    printf("\n=== Ví dụ 7: Exec chương trình với input ===\n\n");
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Thực thi wc để đếm số dòng
        printf("Thực thi: execlp(\"wc\", \"wc\", \"-l\", \"/etc/passwd\", NULL)\n\n");
        execlp("wc", "wc", "-l", "/etc/passwd", NULL);
        
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        int status;
        wait(&status);
        
        if (WIFEXITED(status)) {
            printf("Con kết thúc với exit code: %d\n", WEXITSTATUS(status));
        }
    }
}

int main() {
    printf("=== EXEC FAMILY EXAMPLES ===\n");
    printf("PID: %d\n", getpid());
    
    example1_execl();
    sleep(1);
    
    example2_execlp();
    sleep(1);
    
    example3_execv();
    sleep(1);
    
    example4_execvp();
    sleep(1);
    
    example5_execle();
    sleep(1);
    
    example6_exec_shell_script();
    sleep(1);
    
    example7_exec_with_input();
    
    printf("\n=== HOÀN THÀNH ===\n");
    printf("\nTóm tắt exec family:\n");
    printf("  execl()  - list of args, full path\n");
    printf("  execlp() - list of args, search PATH\n");
    printf("  execle() - list of args, custom env\n");
    printf("  execv()  - array of args, full path\n");
    printf("  execvp() - array of args, search PATH\n");
    printf("  execvpe()- array of args, PATH, custom env\n");
    
    return 0;
}
