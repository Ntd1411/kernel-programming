CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2 -std=c11
TARGET = mutex_unlock

all: $(TARGET)

$(TARGET): mutex_unlock.c
	$(CC) $(CFLAGS) -o $(TARGET) mutex_unlock.c

clean:
	rm -f $(TARGET)

test: $(TARGET)
	@echo "Running mutex unlock demo..."
	./$(TARGET)

.PHONY: all clean test
