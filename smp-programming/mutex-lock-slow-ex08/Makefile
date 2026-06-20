CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2 -std=c11
TARGET = mutex_lock_slow

all: $(TARGET)

$(TARGET): mutex_lock_slow.c
	$(CC) $(CFLAGS) -o $(TARGET) mutex_lock_slow.c

clean:
	rm -f $(TARGET)

test: $(TARGET)
	@echo "Running mutex lock slow path demo..."
	./$(TARGET)

.PHONY: all clean test
