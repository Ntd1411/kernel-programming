# Makefile cho Process Management examples

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
LDFLAGS = -pthread -lrt

TARGETS = fork_example exec_family signal_handler zombie_reaper process_priority

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

process_priority: process_priority.c
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Build thành công: $@"

clean:
	rm -f $(TARGETS) *.o
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
	@echo "   ./signal_handler"
	@echo ""
	@echo "4. Zombie Reaper:"
	@echo "   ./zombie_reaper"
	@echo ""
	@echo "5. Process Priority:"
	@echo "   ./process_priority"

help:
	@echo "Makefile cho Process Management Examples"
	@echo ""
	@echo "Targets:"
	@echo "  all              - Build tất cả chương trình"
	@echo "  fork_example     - Build fork example"
	@echo "  exec_family      - Build exec family example"
	@echo "  signal_handler   - Build signal handler"
	@echo "  zombie_reaper    - Build zombie reaper"
	@echo "  process_priority - Build process priority demo"
	@echo "  clean            - Xóa các file đã build"
	@echo "  test             - Hiển thị hướng dẫn test"
	@echo "  help             - Hiển thị help này"
