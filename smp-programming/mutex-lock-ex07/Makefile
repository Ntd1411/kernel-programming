CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2 -std=c11
TARGET = mutex_lock

all: $(TARGET)

$(TARGET): mutex_lock.c
	$(CC) $(CFLAGS) -o $(TARGET) mutex_lock.c

clean:
	rm -f $(TARGET)

test: $(TARGET)
	@echo "Running mutex lock fast path demo..."
	./$(TARGET)

.PHONY: all clean test
