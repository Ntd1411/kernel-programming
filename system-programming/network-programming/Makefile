# Makefile cho Network Programming examples

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
TARGETS = select_server poll_server epoll_server raw_socket packet_sniffer network_interface

.PHONY: all clean test help

all: $(TARGETS)

select_server: select_server.c
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Build thành công: $@"

poll_server: poll_server.c
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Build thành công: $@"

epoll_server: epoll_server.c
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Build thành công: $@"

raw_socket: raw_socket.c
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Build thành công: $@"
	@echo "Chú ý: Cần quyền root để chạy"

packet_sniffer: packet_sniffer.c
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Build thành công: $@"
	@echo "Chú ý: Cần quyền root để chạy"

network_interface: network_interface.c
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Build thành công: $@"
	@echo "Chú ý: Một số chức năng cần quyền root (set IP, up/down interface)"

clean:
	rm -f $(TARGETS) *.o
	@echo "Đã dọn dẹp"

test: all
	@echo "=== Test các chương trình ==="
	@echo ""
	@echo "1. Test Select Server:"
	@echo "   Terminal 1: ./select_server 8080"
	@echo "   Terminal 2: telnet localhost 8080"
	@echo ""
	@echo "2. Test Poll Server:"
	@echo "   Terminal 1: ./poll_server 8081"
	@echo "   Terminal 2: telnet localhost 8081"
	@echo ""
	@echo "3. Test Epoll Server:"
	@echo "   Terminal 1: ./epoll_server 8082"
	@echo "   Terminal 2: telnet localhost 8082"
	@echo ""
	@echo "4. Test Raw Socket (cần root):"
	@echo "   sudo ./raw_socket 8.8.8.8"
	@echo ""
	@echo "5. Test Packet Sniffer (cần root):"
	@echo "   sudo ./packet_sniffer eth0"
	@echo ""
	@echo "6. Test Network Interface:"
	@echo "   ./network_interface           # Chế độ read-only"
	@echo "   sudo ./network_interface      # Chế độ đầy đủ (read/write)"

help:
	@echo "Makefile cho Network Programming Examples"
	@echo ""
	@echo "Targets:"
	@echo "  all              - Build tất cả chương trình"
	@echo "  select_server    - Build select server"
	@echo "  poll_server      - Build poll server"
	@echo "  epoll_server     - Build epoll server"
	@echo "  raw_socket       - Build raw socket example"
	@echo "  packet_sniffer   - Build packet sniffer"
	@echo "  network_interface - Build network interface management tool"
	@echo "  clean            - Xóa các file đã build"
	@echo "  test             - Hiển thị hướng dẫn test"
	@echo "  help             - Hiển thị help này"
	@echo ""
	@echo "Sử dụng:"
	@echo "  make              # Build tất cả"
	@echo "  make clean        # Dọn dẹp"
	@echo "  make select_server  # Build chỉ select_server"
