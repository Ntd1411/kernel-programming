# Makefile cho Process Management examples

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
LDFLAGS = -pthread -lrt

TARGETS = fork_example exec_family signal_handler zombie_reaper daemon \
          process_priority process_groups process_info cpu_affinity ipc_basics

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

process_priority: process_priority.c
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Build thành công: $@"

process_groups: process_groups.c
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Build thành công: $@"

process_info: process_info.c
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Build thành công: $@"

cpu_affinity: cpu_affinity.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<
	@echo "Build thành công: $@"

ipc_basics: ipc_basics.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<
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
	@echo "   ./signal_handler"
	@echo ""
	@echo "4. Zombie Reaper:"
	@echo "   ./zombie_reaper"
	@echo ""
	@echo "5. Daemon:"
	@echo "   ./daemon start"
	@echo "   ./daemon status"
	@echo "   ./daemon stop"
	@echo ""
	@echo "6. Process Priority:"
	@echo "   ./process_priority"
	@echo ""
	@echo "7. Process Groups:"
	@echo "   ./process_groups"
	@echo ""
	@echo "8. Process Info:"
	@echo "   ./process_info"
	@echo ""
	@echo "9. CPU Affinity:"
	@echo "   ./cpu_affinity"
	@echo ""
	@echo "10. IPC Basics:"
	@echo "   ./ipc_basics"

help:
	@echo "Makefile cho Process Management Examples"
	@echo ""
	@echo "Targets:"
	@echo "  all              - Build tất cả chương trình"
	@echo "  fork_example     - Build fork example"
	@echo "  exec_family      - Build exec family example"
	@echo "  signal_handler   - Build signal handler"
	@echo "  zombie_reaper    - Build zombie reaper"
	@echo "  daemon           - Build daemon example"
	@echo "  process_priority - Build process priority demo"
	@echo "  process_groups   - Build process groups demo"
	@echo "  process_info     - Build process info demo"
	@echo "  cpu_affinity     - Build CPU affinity demo"
	@echo "  ipc_basics       - Build IPC basics demo"
	@echo "  clean            - Xóa các file đã build"
	@echo "  test             - Hiển thị hướng dẫn test"
	@echo "  help             - Hiển thị help này"
