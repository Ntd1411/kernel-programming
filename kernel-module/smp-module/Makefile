# Makefile — SMP Synchronization Kernel Module

# Tên object file (phải trùng với tên file .c)
obj-m += smp_sync.o

# Extra compiler flags (tuỳ chọn)
ccflags-y := -Wall -Wextra -DDEBUG

# Đường dẫn kernel headers của kernel đang chạy
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

# ── Targets ──────────────────────────────────────────────
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# Tiện ích: load / unload / đọc proc
load:
	sudo insmod smp_sync.ko
	@echo "Module loaded. Kernel log:"
	@sudo dmesg | tail -20

unload:
	sudo rmmod smp_sync
	@echo "Module unloaded. Kernel log:"
	@sudo dmesg | tail -10

read:
	@sudo cat /proc/smp_sync

log:
	@sudo dmesg | grep smp_sync | tail -30

# Build + load trong một bước
install: all load

.PHONY: all clean load unload read log install