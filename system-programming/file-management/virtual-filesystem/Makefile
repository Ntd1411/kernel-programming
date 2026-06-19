# Makefile for SimplFS Kernel Module

obj-m += vfs_module.o

# Lay kernel build directory
KDIR := /lib/modules/$(shell uname -r)/build

# Thu muc hien tai
PWD := $(shell pwd)

# Flags
ccflags-y := -std=gnu99 -Wno-declaration-after-statement

all:
	@echo "Building SimplFS kernel module..."
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	@echo "Build complete. Module: vfs_module.ko"

clean:
	@echo "Cleaning build files..."
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f *.o *.ko *.mod.c *.mod *.order *.symvers
	@echo "Clean complete."

install: all
	@echo "Installing module..."
	sudo insmod vfs_module.ko
	@echo "Module installed. Check with: lsmod | grep vfs_module"
	@echo "View kernel log: dmesg | tail -20"

uninstall:
	@echo "Uninstalling module..."
	sudo rmmod vfs_module
	@echo "Module uninstalled."
	@echo "View kernel log: dmesg | tail -20"

test: install
	@echo ""
	@echo "=== Testing SimplFS ==="
	@echo "1. Creating mount point..."
	sudo mkdir -p /mnt/simplefs
	@echo ""
	@echo "2. Mounting filesystem..."
	sudo mount -t simplefs none /mnt/simplefs
	@echo ""
	@echo "3. Listing files..."
	ls -la /mnt/simplefs
	@echo ""
	@echo "4. Reading hello file..."
	cat /mnt/simplefs/hello
	@echo ""
	@echo "5. Reading info file..."
	cat /mnt/simplefs/info
	@echo ""
	@echo "6. Checking filesystem type..."
	mount | grep simplefs
	@echo ""
	@echo "7. Getting filesystem stats..."
	df -h /mnt/simplefs
	stat -f /mnt/simplefs
	@echo ""
	@echo "8. Unmounting..."
	sudo umount /mnt/simplefs
	@echo ""
	@echo "Test complete!"

reload: uninstall clean all install

help:
	@echo "SimplFS Kernel Module Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build the kernel module (default)"
	@echo "  clean     - Remove build files"
	@echo "  install   - Build and insert module into kernel"
	@echo "  uninstall - Remove module from kernel"
	@echo "  test      - Run full test suite"
	@echo "  reload    - Uninstall, clean, rebuild and install"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Usage examples:"
	@echo "  make              # Build module"
	@echo "  make install      # Install module"
	@echo "  make test         # Run tests"
	@echo "  make uninstall    # Remove module"
	@echo "  make clean        # Clean build files"

.PHONY: all clean install uninstall test reload help
