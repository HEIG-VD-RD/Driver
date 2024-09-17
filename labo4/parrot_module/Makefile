# Default value for USE_VM
USE_VM ?= 1

ifeq ($(USE_VM),1)
    KERNELDIR := /home/reds/linux-socfpga/
    TOOLCHAIN := /opt/toolchains/arm-linux-gnueabihf_6.4.1/bin/arm-linux-gnueabihf-
else
    KERNELDIR := /home/rafou/Desktop/heig-vd/DRV/linux-socfpga/
    TOOLCHAIN := /home/rafou/Desktop/heig-vd/DRV/gcc-linaro-6.4.1-2018.05-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
endif


obj-m := parrot.o

PWD := $(shell pwd)
WARN := -W -Wall -Wstrict-prototypes -Wmissing-prototypes

all: parrot parrot_test

parrot_test:
	@echo "Building userspace test application"
	$(TOOLCHAIN)gcc -o $@ parrot_test.c -Wall
	cp $@ /export/drv	

parrot:
	@echo "Building with kernel sources in $(KERNELDIR)"
	$(MAKE) ARCH=arm CROSS_COMPILE=$(TOOLCHAIN) -C $(KERNELDIR) M=$(PWD) ${WARN}
	cp $@.ko /export/drv
clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions modules.order Module.symvers
	rm parrot_test
