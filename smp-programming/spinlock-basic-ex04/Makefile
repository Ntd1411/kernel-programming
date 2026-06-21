CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2 -std=c11
TARGET = spinlock_basic

all: $(TARGET)

$(TARGET): spinlock_basic.c
	$(CC) $(CFLAGS) -o $(TARGET) spinlock_basic.c

clean:
	rm -f $(TARGET)

test: $(TARGET)
	@echo "Running basic spin lock test..."
	./$(TARGET)

.PHONY: all clean test
