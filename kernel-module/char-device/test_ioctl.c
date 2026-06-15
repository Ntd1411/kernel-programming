/*
 * test_ioctl.c - User space test program cho ioctl device
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#define DEVICE_PATH "/dev/mychardev_ioctl"

#define IOCTL_MAGIC 'k'
#define IOCTL_GET_SIZE _IOR(IOCTL_MAGIC, 1, int)
#define IOCTL_CLEAR_BUFFER _IO(IOCTL_MAGIC, 2)
#define IOCTL_GET_CPU _IOR(IOCTL_MAGIC, 3, int)
#define IOCTL_GET_STATS _IOR(IOCTL_MAGIC, 4, struct device_stats)

struct device_stats {
    int read_count;
    int write_count;
    int ioctl_count;
    int current_size;
};

int main(void) {
    int fd;
    int size, cpu;
    struct device_stats stats;
    char buffer[256];
    
    printf("=== Test IOCTL Device ===\n\n");
    
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Loi khi mo device");
        printf("Hay dam bao module da duoc load: sudo insmod chardev_ioctl.ko\n");
        return 1;
    }
    
    printf("1. Test GET_CPU\n");
    if (ioctl(fd, IOCTL_GET_CPU, &cpu) == 0) {
        printf("   Device dang chay tren CPU: %d\n\n", cpu);
    } else {
        perror("   Loi IOCTL_GET_CPU");
    }
    
    printf("2. Test WRITE va GET_SIZE\n");
    strcpy(buffer, "Test data for ioctl device");
    write(fd, buffer, strlen(buffer));
    printf("   Da ghi: %s\n", buffer);
    
    if (ioctl(fd, IOCTL_GET_SIZE, &size) == 0) {
        printf("   Kich thuoc buffer: %d bytes\n\n", size);
    } else {
        perror("   Loi IOCTL_GET_SIZE");
    }
    
    printf("3. Test READ\n");
    lseek(fd, 0, SEEK_SET);
    memset(buffer, 0, sizeof(buffer));
    read(fd, buffer, 256);
    printf("   Da doc: %s\n\n", buffer);
    
    printf("4. Test GET_STATS\n");
    if (ioctl(fd, IOCTL_GET_STATS, &stats) == 0) {
        printf("   So lan doc: %d\n", stats.read_count);
        printf("   So lan ghi: %d\n", stats.write_count);
        printf("   So lan ioctl: %d\n", stats.ioctl_count);
        printf("   Kich thuoc hien tai: %d bytes\n\n", stats.current_size);
    } else {
        perror("   Loi IOCTL_GET_STATS");
    }
    
    printf("5. Test CLEAR_BUFFER\n");
    if (ioctl(fd, IOCTL_CLEAR_BUFFER) == 0) {
        printf("   Da xoa buffer\n");
        
        if (ioctl(fd, IOCTL_GET_SIZE, &size) == 0) {
            printf("   Kich thuoc buffer sau khi xoa: %d bytes\n\n", size);
        }
    } else {
        perror("   Loi IOCTL_CLEAR_BUFFER");
    }
    
    printf("6. Test GET_STATS lan cuoi\n");
    if (ioctl(fd, IOCTL_GET_STATS, &stats) == 0) {
        printf("   So lan doc: %d\n", stats.read_count);
        printf("   So lan ghi: %d\n", stats.write_count);
        printf("   So lan ioctl: %d\n", stats.ioctl_count);
        printf("   Kich thuoc hien tai: %d bytes\n", stats.current_size);
    }
    
    close(fd);
    printf("\n=== Hoan thanh test ===\n");
    
    return 0;
}
