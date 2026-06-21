CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2
TARGET = irq_disable

all: $(TARGET)

$(TARGET): irq_disable.c
	$(CC) $(CFLAGS) -o $(TARGET) irq_disable.c

clean:
	rm -f $(TARGET)

test: $(TARGET)
	@echo "Running IRQ disable/enable demo..."
	./$(TARGET)

.PHONY: all clean test
