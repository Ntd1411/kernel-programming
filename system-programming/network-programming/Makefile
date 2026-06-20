# Makefile cho Network Programming examples

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g

# Kernel modules
obj-m += loopback_driver.o
obj-m += http_password_detector.o
obj-m += skbuff_demo.o
obj-m += tcp_steganography.o

# User-space programs
USER_TARGETS = network_interface stego_reader

.PHONY: all modules user clean help test

all: modules user

# Build kernel modules
modules:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	@echo "Build kernel modules thành công"

# Build user-space programs
user: $(USER_TARGETS)

stego_reader: stego_reader.c
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Build user-space program thành công: $@"

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f $(USER_TARGETS) *.o
	@echo "Đã dọn dẹp"

help:
	@echo "========================================="
	@echo "Network Programming - Kernel Modules"
	@echo "========================================="
	@echo ""
	@echo "Targets:"
	@echo "  all              - Build tất cả (modules + user programs)"
	@echo "  modules          - Build kernel modules"
	@echo "  user             - Build user-space programs"
	@echo "  clean            - Xóa các file đã build"
	@echo "  test             - Hiển thị hướng dẫn test"
	@echo "  help             - Hiển thị help này"
	@echo ""
	@echo "Kernel Modules:"
	@echo "  loopback_driver.ko          - Loopback network driver (Slide 13.1)"
	@echo "  http_password_detector.ko   - HTTP password detector (Slide 13.2)"
	@echo "  skbuff_demo.ko              - sk_buff demo (Slide 7-8)"
	@echo "  tcp_steganography.ko        - TCP/UDP steganography (ẩn tin trong packets)"
	@echo ""
	@echo "User Programs:"
	@echo "  stego_reader                - Steganography reader (đọc tin đã ẩn)"

test:
	@echo ""
	@echo "========================================="
	@echo "Hướng dẫn Test"
	@echo "========================================="
	@echo ""
	@echo "=== 1. Test Loopback Driver (Slide 13.1) ==="
	@echo "Load module:"
	@echo "  sudo insmod loopback_driver.ko"
	@echo ""
	@echo "Cấu hình interface:"
	@echo "  sudo ip link set myloop0 up"
	@echo "  sudo ip addr add 10.0.0.1/24 dev myloop0"
	@echo ""
	@echo "Test ping (loopback):"
	@echo "  ping -c 3 10.0.0.2"
	@echo ""
	@echo "Xem log:"
	@echo "  dmesg | tail -20"
	@echo ""
	@echo "Unload:"
	@echo "  sudo rmmod loopback_driver"
	@echo ""
	@echo "=== 2. Test HTTP Password Detector (Slide 13.2) ==="
	@echo "Load module:"
	@echo "  sudo insmod http_password_detector.ko"
	@echo ""
	@echo "Test với curl:"
	@echo "  curl -X POST -d \"username=admin&password=secret123\" http://httpbin.org/post"
	@echo "  curl -X POST -d \"user=john&password=mypass456&email=test\" http://example.com"
	@echo ""
	@echo "Xem log realtime:"
	@echo "  sudo tail -f /var/log/http_passwords.log"
	@echo "  dmesg | grep -i password"
	@echo ""
	@echo "Unload:"
	@echo "  sudo rmmod http_password_detector"
	@echo ""
	@echo "=== 3. Test sk_buff Demo (Slide 7-8) ==="
	@echo "Load module (tự động chạy demo):"
	@echo "  sudo insmod skbuff_demo.ko"
	@echo ""
	@echo "Xem kết quả:"
	@echo "  dmesg | tail -100"
	@echo ""
	@echo "Unload:"
	@echo "  sudo rmmod skbuff_demo"
	@echo ""
	@echo "=== 4. Test Network Interface Tool ==="
	@echo "Chạy (read-only mode):"
	@echo "  ./network_interface"
	@echo ""
	@echo "Chạy với quyền root (full features):"
	@echo "  sudo ./network_interface"
	@echo ""
	@echo "=== 5. Test TCP/UDP Steganography ==="
	@echo "Load module với custom message:"
	@echo "  sudo insmod tcp_steganography.ko message=\"SECRET_MESSAGE\""
	@echo ""
	@echo "Terminal 2 - Chạy stego_reader:"
	@echo "  sudo ./stego_reader eth0"
	@echo ""
	@echo "Terminal 1 - Tạo traffic để ẩn tin:"
	@echo "  ping -c 10 google.com"
	@echo "  curl http://example.com"
	@echo "  wget http://httpbin.org/get"
	@echo ""
	@echo "Xem log kernel:"
	@echo "  dmesg | grep -i stego"
	@echo ""
	@echo "Unload:"
	@echo "  sudo rmmod tcp_steganography"
	@echo ""
	@echo "========================================="
