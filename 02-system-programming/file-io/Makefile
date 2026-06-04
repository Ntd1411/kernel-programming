# Makefile for file-io examples

CC = gcc
CFLAGS = -Wall -Wextra -g -O2
LDFLAGS = 

PROGRAMS = file_operations file_locking memory_map directory_walk inotify_example

.PHONY: all clean test

all: $(PROGRAMS)

file_operations: file_operations.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

file_locking: file_locking.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

memory_map: memory_map.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

directory_walk: directory_walk.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

inotify_example: inotify_example.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(PROGRAMS) *.o
	rm -f test*.txt test*.dat
	rm -f *.log

test: all
	@echo "=== Testing file_operations ==="
	./file_operations write test1.txt "Hello World"
	./file_operations read test1.txt
	./file_operations append test1.txt "\nNew line"
	./file_operations read test1.txt
	./file_operations stat test1.txt
	
	@echo "\n=== Testing directory_walk ==="
	./directory_walk list .
	./directory_walk count .
	
	@echo "\n=== Build hoàn tất ==="
	@echo "Các chương trình: $(PROGRAMS)"
