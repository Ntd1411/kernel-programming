CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2 -std=c11
TARGET = per_cpu_data

all: $(TARGET)

$(TARGET): per_cpu_data.c
	$(CC) $(CFLAGS) -o $(TARGET) per_cpu_data.c

clean:
	rm -f $(TARGET)

test: $(TARGET)
	@echo "Running per-CPU data demo..."
	./$(TARGET)

.PHONY: all clean test
