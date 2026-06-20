CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2
TARGET = race_condition

all: $(TARGET)

$(TARGET): race_condition.c
	$(CC) $(CFLAGS) -o $(TARGET) race_condition.c

clean:
	rm -f $(TARGET)

test: $(TARGET)
	@echo "Running race condition test (may need multiple runs to trigger)..."
	@for i in 1 2 3 4 5; do \
		echo "\n=== Run $$i ==="; \
		./$(TARGET) || true; \
	done

.PHONY: all clean test
