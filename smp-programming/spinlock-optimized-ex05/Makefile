CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2 -std=c11
TARGET = spinlock_optimized

all: $(TARGET)

$(TARGET): spinlock_optimized.c
	$(CC) $(CFLAGS) -o $(TARGET) spinlock_optimized.c

clean:
	rm -f $(TARGET)

test: $(TARGET)
	@echo "Running optimized spin lock comparison..."
	./$(TARGET)

.PHONY: all clean test
