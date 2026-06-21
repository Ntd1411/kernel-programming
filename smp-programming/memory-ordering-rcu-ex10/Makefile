CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2 -std=gnu11
TARGET = memory_ordering_rcu

all: $(TARGET)

$(TARGET): memory_ordering_rcu.c
	$(CC) $(CFLAGS) -o $(TARGET) memory_ordering_rcu.c

clean:
	rm -f $(TARGET)

test: $(TARGET)
	@echo "Running memory ordering and RCU demo..."
	./$(TARGET)

.PHONY: all clean test
