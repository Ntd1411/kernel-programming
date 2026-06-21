CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2 -std=c11
TARGET = semaphore

all: $(TARGET)

$(TARGET): semaphore.c
	$(CC) $(CFLAGS) -o $(TARGET) semaphore.c

clean:
	rm -f $(TARGET)

test: $(TARGET)
	@echo "Running semaphore demo..."
	./$(TARGET)

.PHONY: all clean test
