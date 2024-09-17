### Put here the path to kernel sources! ###
KERNELDIR := /home/rafou/Desktop/heig-vd/DRV/linux-socfpga/
TOOLCHAIN := /home/rafou/Desktop/heig-vd/DRV/gcc-linaro-6.4.1-2018.05-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-

GCC := $(TOOLCHAIN)gcc

obj-m := flifo.o

PWD := $(shell pwd)
WARN := -W -Wall -Wstrict-prototypes -Wmissing-prototypes

all: flifo test deploy

flifo:
	@echo "Building with kernel sources in $(KERNELDIR)"
	$(MAKE) ARCH=arm CROSS_COMPILE=$(TOOLCHAIN) -C $(KERNELDIR) M=$(PWD) ${WARN}
test:
	$(GCC) flifo_test.c -o flifo_test
deploy:
	cp flifo.ko flifo_test /export/drv	

clean:
	rm -rf flifo_test *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions modules.order Module.symvers
