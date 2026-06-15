# Makefile cho pthread-basics

CC = gcc
CFLAGS = -Wall -Wextra -g -pthread
LDFLAGS = -pthread

TARGETS = thread_create thread_args thread_return thread_detach thread_cancel

all: $(TARGETS)

thread_create: thread_create.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

thread_args: thread_args.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

thread_return: thread_return.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

thread_detach: thread_detach.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

thread_cancel: thread_cancel.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGETS) *.o

run: all
	@echo "Chạy các chương trình..."
	@echo "\n=== thread_create ==="
	./thread_create
	@echo "\n=== thread_args ==="
	./thread_args
	@echo "\n=== thread_return ==="
	./thread_return
	@echo "\n=== thread_detach ==="
	./thread_detach
	@echo "\n=== thread_cancel ==="
	./thread_cancel

.PHONY: all clean run
