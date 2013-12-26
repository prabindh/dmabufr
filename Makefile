-include Rules.make

TARGET = dmabufr.ko
obj-m = dmabufr.o

peemuperf-objs = dmabufr.o

MAKE_ENV = ARCH=arm CROSS_COMPILE=$(TOOLCHAIN_PATH)

.PHONY: release

default: release

release:
	make -C $(LINUXKERNEL_INSTALL_DIR) M=`pwd` $(MAKE_ENV) modules

