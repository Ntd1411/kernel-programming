# Makefile cho kernel modules

obj-m += hello_module.o
obj-m += param_module.o
obj-m += smp_info_module.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	depmod -a

load-hello:
	sudo insmod hello_module.ko
	dmesg | tail -n 10

unload-hello:
	sudo rmmod hello_module
	dmesg | tail -n 5

load-param:
	sudo insmod param_module.ko name="Linux" count=3 verbose=1
	dmesg | tail -n 15

unload-param:
	sudo rmmod param_module
	dmesg | tail -n 5

load-smp:
	sudo insmod smp_info_module.ko
	dmesg | tail -n 30

unload-smp:
	sudo rmmod smp_info_module
	dmesg | tail -n 5

test-all: load-hello load-param load-smp
	@echo "Tat ca module da duoc load. Kiem tra voi: lsmod | grep module"

clean-all: unload-hello unload-param unload-smp
	@echo "Tat ca module da duoc go bo"

info:
	@echo "Cac module co san:"
	@echo "  - hello_module.ko: Module co ban"
	@echo "  - param_module.ko: Module voi parameters"
	@echo "  - smp_info_module.ko: Module hien thi thong tin SMP"
	@echo ""
	@echo "Cach su dung:"
	@echo "  make            - Bien dich tat ca module"
	@echo "  make clean      - Xoa cac file bien dich"
	@echo "  make load-hello - Load hello_module"
	@echo "  make load-param - Load param_module voi tham so"
	@echo "  make load-smp   - Load smp_info_module"
	@echo "  make test-all   - Load tat ca module"
	@echo "  make clean-all  - Unload tat ca module"

.PHONY: all clean install load-hello unload-hello load-param unload-param load-smp unload-smp test-all clean-all info
