CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2 -std=c11
TARGET = atomic_fix

all: $(TARGET)

$(TARGET): atomic_fix.c
	$(CC) $(CFLAGS) -o $(TARGET) atomic_fix.c

clean:
	rm -f $(TARGET)

test: $(TARGET)
	@echo "Running atomic fix test (should always succeed)..."
	@for i in 1 2 3 4 5; do \
		echo "\n=== Run $$i ==="; \
		./$(TARGET); \
	done

.PHONY: all clean test
