# Makefile cho Process Management examples

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g

TARGETS = fork_example exec_family signal_handler zombie_reaper daemon

.PHONY: all clean test help

all: $(TARGETS)

fork_example: fork_example.c
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Build thành công: $@"

exec_family: exec_family.c
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Build thành công: $@"

signal_handler: signal_handler.c
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Build thành công: $@"

zombie_reaper: zombie_reaper.c
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Build thành công: $@"

daemon: daemon.c
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Build thành công: $@"

clean:
	rm -f $(TARGETS) *.o
	rm -f /tmp/my_daemon.pid /tmp/my_daemon.log
	@echo "Đã dọn dẹp"

test: all
	@echo "=== Hướng dẫn test các chương trình ==="
	@echo ""
	@echo "1. Fork Example:"
	@echo "   ./fork_example"
	@echo ""
	@echo "2. Exec Family:"
	@echo "   ./exec_family"
	@echo ""
	@echo "3. Signal Handler:"
	@echo "   ./signal_handler 1  # Basic signal"
	@echo "   ./signal_handler 2  # sigaction"
	@echo "   ./signal_handler 3  # alarm"
	@echo "   ./signal_handler 4  # blocking"
	@echo "   ./signal_handler 5  # send signal"
	@echo ""
	@echo "4. Zombie Reaper:"
	@echo "   ./zombie_reaper 1   # Create zombie"
	@echo "   ./zombie_reaper 2   # Auto-reap"
	@echo "   ./zombie_reaper 3   # waitpid WNOHANG"
	@echo "   ./zombie_reaper 4   # Wait specific"
	@echo "   ./zombie_reaper 5   # Double fork"
	@echo ""
	@echo "5. Daemon:"
	@echo "   ./daemon start      # Start daemon"
	@echo "   ./daemon status     # Check status"
	@echo "   tail -f /tmp/my_daemon.log  # View log"
	@echo "   ./daemon stop       # Stop daemon"

help:
	@echo "Makefile cho Process Management Examples"
	@echo ""
	@echo "Targets:"
	@echo "  all            - Build tất cả chương trình"
	@echo "  fork_example   - Build fork example"
	@echo "  exec_family    - Build exec family example"
	@echo "  signal_handler - Build signal handler"
	@echo "  zombie_reaper  - Build zombie reaper"
	@echo "  daemon         - Build daemon example"
	@echo "  clean          - Xóa các file đã build"
	@echo "  test           - Hiển thị hướng dẫn test"
	@echo "  help           - Hiển thị help này"
