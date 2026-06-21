CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2 -std=c11
TARGET = preemption_counter

all: $(TARGET)

$(TARGET): preemption_counter.c
	$(CC) $(CFLAGS) -o $(TARGET) preemption_counter.c

clean:
	rm -f $(TARGET)

test: $(TARGET)
	@echo "Running preemption counter demo..."
	./$(TARGET)

.PHONY: all clean test
