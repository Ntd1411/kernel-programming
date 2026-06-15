/*
 * test_chardev.c - User space test program cho character device
 * 
 * Chuong trinh test cac chuc nang cua character device
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#define DEVICE_PATH "/dev/mychardev"
#define BUFFER_SIZE 1024

void test_basic_operations(void) {
    int fd;
    char write_buffer[BUFFER_SIZE];
    char read_buffer[BUFFER_SIZE];
    ssize_t bytes;
    
    printf("\n=== Test 1: Cac thao tac co ban ===\n");
    
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Loi khi mo device");
        return;
    }
    printf("Mo device thanh cong\n");
    
    strcpy(write_buffer, "Hello from user space!");
    bytes = write(fd, write_buffer, strlen(write_buffer));
    if (bytes < 0) {
        perror("Loi khi ghi du lieu");
    } else {
        printf("Da ghi %ld bytes: %s\n", bytes, write_buffer);
    }
    
    lseek(fd, 0, SEEK_SET);
    
    memset(read_buffer, 0, BUFFER_SIZE);
    bytes = read(fd, read_buffer, BUFFER_SIZE);
    if (bytes < 0) {
        perror("Loi khi doc du lieu");
    } else {
        printf("Da doc %ld bytes: %s\n", bytes, read_buffer);
    }
    
    close(fd);
    printf("Dong device thanh cong\n");
}

void test_multiple_writes(void) {
    int fd;
    char buffer[BUFFER_SIZE];
    int i;
    
    printf("\n=== Test 2: Nhieu lan ghi lien tiep ===\n");
    
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Loi khi mo device");
        return;
    }
    
    for (i = 0; i < 5; i++) {
        snprintf(buffer, BUFFER_SIZE, "Dong thu %d tu user space", i + 1);
        write(fd, buffer, strlen(buffer));
        printf("Ghi lan %d: %s\n", i + 1, buffer);
        sleep(1);
    }
    
    close(fd);
}

void test_concurrent_access(void) {
    int fd1, fd2;
    
    printf("\n=== Test 3: Truy cap dong thoi ===\n");
    
    fd1 = open(DEVICE_PATH, O_RDWR);
    if (fd1 < 0) {
        perror("Loi khi mo device lan 1");
        return;
    }
    printf("Process 1 mo device thanh cong\n");
    
    fd2 = open(DEVICE_PATH, O_RDWR);
    if (fd2 < 0) {
        printf("Process 2 khong the mo device (mong doi - device da khoa)\n");
    } else {
        printf("Process 2 mo device thanh cong (khong mong doi)\n");
        close(fd2);
    }
    
    close(fd1);
    printf("Process 1 dong device\n");
    
    fd2 = open(DEVICE_PATH, O_RDWR);
    if (fd2 < 0) {
        perror("Loi khi mo device lan 2");
    } else {
        printf("Process 2 co the mo device sau khi process 1 dong\n");
        close(fd2);
    }
}

void test_large_data(void) {
    int fd;
    char *large_buffer;
    ssize_t bytes;
    
    printf("\n=== Test 4: Ghi du lieu lon ===\n");
    
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Loi khi mo device");
        return;
    }
    
    large_buffer = malloc(2048);
    memset(large_buffer, 'A', 2048);
    large_buffer[2047] = '\0';
    
    bytes = write(fd, large_buffer, 2048);
    printf("Thu ghi 2048 bytes, thuc te ghi duoc: %ld bytes\n", bytes);
    
    free(large_buffer);
    close(fd);
}

int main(int argc, char *argv[]) {
    printf("=== Chuong trinh test Character Device ===\n");
    printf("Device: %s\n", DEVICE_PATH);
    
    if (access(DEVICE_PATH, F_OK) != 0) {
        printf("Loi: Device %s khong ton tai\n", DEVICE_PATH);
        printf("Hay dam bao module da duoc load: sudo insmod chardev.ko\n");
        return 1;
    }
    
    test_basic_operations();
    test_multiple_writes();
    test_concurrent_access();
    test_large_data();
    
    printf("\n=== Hoan thanh tat ca cac test ===\n");
    printf("Kiem tra kernel log: sudo dmesg | tail -n 50\n");
    
    return 0;
}
