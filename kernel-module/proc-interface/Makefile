# Makefile cho Proc Interface modules

obj-m += proc_basic.o
obj-m += proc_seq.o
obj-m += proc_rw.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f *.markers

install:
	sudo insmod proc_basic.ko
	sudo insmod proc_seq.ko
	sudo insmod proc_rw.ko

uninstall:
	sudo rmmod proc_rw
	sudo rmmod proc_seq
	sudo rmmod proc_basic

test: all install
	@echo "=== Test Proc Basic ==="
	cat /proc/myproc
	@echo ""
	@echo "=== Test Proc Seq ==="
	cat /proc/cpu_info
	@echo ""
	@echo "=== Test Proc RW (Read) ==="
	cat /proc/mymessage
	@echo ""
	@echo "=== Test Proc RW (Write) ==="
	echo "Hello from user space!" | sudo tee /proc/mymessage
	@echo ""
	@echo "=== Test Proc RW (Read after write) ==="
	cat /proc/mymessage
	@echo ""
	@echo "=== Check kernel log ==="
	sudo dmesg | tail -20

help:
	@echo "Makefile cho Proc Interface Modules"
	@echo ""
	@echo "Cac lenh:"
	@echo "  make          - Compile tat ca modules"
	@echo "  make clean    - Xoa cac file da compile"
	@echo "  make install  - Load tat ca modules"
	@echo "  make uninstall- Unload tat ca modules"
	@echo "  make test     - Compile, load va test modules"
	@echo "  make help     - Hien thi thong tin nay"
	@echo ""
	@echo "Test thu cong:"
	@echo "  cat /proc/myproc          - Doc proc basic"
	@echo "  cat /proc/cpu_info        - Doc proc seq"
	@echo "  cat /proc/mymessage       - Doc message"
	@echo "  echo 'text' > /proc/mymessage  - Ghi message"

.PHONY: all clean install uninstall test help
