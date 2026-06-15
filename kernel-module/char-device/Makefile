# Makefile cho character device modules

obj-m += chardev.o
obj-m += chardev_ioctl.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: modules userspace

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

userspace:
	gcc -o test_chardev test_chardev.c
	gcc -o test_ioctl test_ioctl.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f test_chardev test_ioctl

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	depmod -a

load-chardev:
	sudo insmod chardev.ko
	ls -l /dev/mychardev
	dmesg | tail -n 10

unload-chardev:
	sudo rmmod chardev
	dmesg | tail -n 5

test-chardev: load-chardev
	sudo ./test_chardev
	$(MAKE) unload-chardev

load-ioctl:
	sudo insmod chardev_ioctl.ko
	ls -l /dev/mychardev_ioctl
	dmesg | tail -n 10

unload-ioctl:
	sudo rmmod chardev_ioctl
	dmesg | tail -n 5

test-ioctl: load-ioctl
	sudo ./test_ioctl
	$(MAKE) unload-ioctl

test-all: test-chardev test-ioctl

.PHONY: all modules userspace clean install load-chardev unload-chardev test-chardev load-ioctl unload-ioctl test-ioctl test-all
