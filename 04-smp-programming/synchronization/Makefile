# Makefile cho synchronization

CC = gcc
CFLAGS = -Wall -Wextra -g -pthread
LDFLAGS = -pthread -lrt

TARGETS = mutex_basic mutex_deadlock rwlock semaphore condition_var barrier spinlock

all: $(TARGETS)

mutex_basic: mutex_basic.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

mutex_deadlock: mutex_deadlock.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

rwlock: rwlock.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

semaphore: semaphore.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

condition_var: condition_var.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

barrier: barrier.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

spinlock: spinlock.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGETS) *.o

test-mutex: mutex_basic
	@echo "=== Test Mutex Basic ==="
	./mutex_basic

test-deadlock: mutex_deadlock
	@echo "=== Test Deadlock ==="
	./mutex_deadlock

test-rwlock: rwlock
	@echo "=== Test RWLock ==="
	./rwlock

test-semaphore: semaphore
	@echo "=== Test Semaphore ==="
	./semaphore

test-condvar: condition_var
	@echo "=== Test Condition Variable ==="
	./condition_var

test-barrier: barrier
	@echo "=== Test Barrier ==="
	./barrier

test-spinlock: spinlock
	@echo "=== Test Spinlock ==="
	./spinlock

test: all
	@echo "Chạy tất cả tests..."
	@$(MAKE) test-mutex
	@echo ""
	@$(MAKE) test-rwlock
	@echo ""
	@$(MAKE) test-semaphore
	@echo ""
	@$(MAKE) test-condvar
	@echo ""
	@$(MAKE) test-barrier
	@echo ""
	@$(MAKE) test-spinlock

# Detect race conditions với Valgrind Helgrind
helgrind: all
	@echo "Chạy Helgrind để detect race conditions..."
	valgrind --tool=helgrind ./mutex_basic

# Detect thread errors với Valgrind DRD
drd: all
	@echo "Chạy DRD để detect thread errors..."
	valgrind --tool=drd ./mutex_basic

.PHONY: all clean test test-mutex test-deadlock test-rwlock test-semaphore test-condvar test-barrier test-spinlock helgrind drd
