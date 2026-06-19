# Makefile cho Socket Programming examples

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
LDFLAGS = -lpthread

TARGETS = tcp_server tcp_client udp_server udp_client echo_server echo_client

.PHONY: all clean test help

all: $(TARGETS)

tcp_server: tcp_server.c
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Build thành công: $@"

tcp_client: tcp_client.c
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Build thành công: $@"

udp_server: udp_server.c
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Build thành công: $@"

udp_client: udp_client.c
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Build thành công: $@"

echo_server: echo_server.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
	@echo "Build thành công: $@"

echo_client: echo_client.c
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Build thành công: $@"

clean:
	rm -f $(TARGETS) *.o
	@echo "Đã dọn dẹp"

test: all
	@echo "=== Hướng dẫn test ==="
	@echo ""
	@echo "1. Test TCP Server/Client:"
	@echo "   Terminal 1: ./tcp_server 8080"
	@echo "   Terminal 2: ./tcp_client localhost 8080"
	@echo ""
	@echo "2. Test UDP Server/Client:"
	@echo "   Terminal 1: ./udp_server 8081"
	@echo "   Terminal 2: ./udp_client localhost 8081"
	@echo ""
	@echo "3. Test Echo Server/Client (multi-threaded):"
	@echo "   Terminal 1: ./echo_server 8082"
	@echo "   Terminal 2: ./echo_client localhost 8082"
	@echo "   Terminal 3: ./echo_client localhost 8082 (nhiều client đồng thời)"
	@echo ""

help:
	@echo "=== Socket Programming Examples ==="
	@echo ""
	@echo "Các chương trình:"
	@echo "  tcp_server    - TCP server đơn giản (sequential)"
	@echo "  tcp_client    - TCP client cơ bản"
	@echo "  udp_server    - UDP server"
	@echo "  udp_client    - UDP client"
	@echo "  echo_server   - Echo server đa luồng (multi-threaded)"
	@echo "  echo_client   - Echo client cho echo server"
	@echo ""
	@echo "Lệnh make:"
	@echo "  make          - Build tất cả"
	@echo "  make clean    - Xóa các file binary"
	@echo "  make test     - Hiển thị hướng dẫn test"
	@echo "  make help     - Hiển thị trợ giúp này"
	@echo "   Terminal 3: ./tcp_client localhost 8082"
	@echo ""
	@echo "4. Test với telnet:"
	@echo "   telnet localhost 8080"
	@echo ""
	@echo "5. Test với netcat:"
	@echo "   nc localhost 8080"

help:
	@echo "Makefile cho Socket Programming Examples"
	@echo ""
	@echo "Targets:"
	@echo "  all         - Build tất cả chương trình"
	@echo "  tcp_server  - Build TCP server"
	@echo "  tcp_client  - Build TCP client"
	@echo "  udp_server  - Build UDP server"
	@echo "  udp_client  - Build UDP client"
	@echo "  echo_server - Build multi-threaded echo server"
	@echo "  clean       - Xóa các file đã build"
	@echo "  test        - Hiển thị hướng dẫn test"
	@echo "  help        - Hiển thị help này"
