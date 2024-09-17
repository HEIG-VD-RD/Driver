# Default value for USE_VM
USE_VM ?= 1

ifeq ($(USE_VM),1)
    KERNELDIR := /home/reds/linux-socfpga/
    TOOLCHAIN := /opt/toolchains/arm-linux-gnueabihf_6.4.1/bin/arm-linux-gnueabihf-
else
    KERNELDIR := /home/rafou/Desktop/heig-vd/DRV/linux-socfpga/
    TOOLCHAIN := /home/rafou/Desktop/heig-vd/DRV/gcc-linaro-6.4.1-2018.05-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
endif

obj-m := ex1.o

GCC := $(TOOLCHAIN)gcc

PWD := $(shell pwd)
WARN := -W -Wall -Wstrict-prototypes -Wmissing-prototypes

TARGET := ex1
TARGET_TEST := show_number
TARGET_TEST2 := show_number_random

all: clean $(TARGET) $(TARGET_TEST) $(TARGET_TEST2) deploy

$(TARGET):
	@echo "Building with kernel sources in $(KERNELDIR)"
	$(MAKE) ARCH=arm CROSS_COMPILE=$(TOOLCHAIN) -C $(KERNELDIR) M=$(PWD) ${WARN}
	rm -rf *.o *~ core .depend .*.cmd *.mod *.mod.c .tmp_versions modules.order Module.symvers *.a

$(TARGET_TEST):
	$(GCC) $@.c -o $@

$(TARGET_TEST2):
	$(GCC) $@.c -o $@
	

deploy:
	cp $(TARGET).ko $(TARGET_TEST) $(TARGET_TEST2) /export/drv
clean:
	rm -rf $(TARGET_TEST) $(TARGET_TEST2) *.o *~ core .depend .*.cmd *.ko *.mod *.mod.c .tmp_versions modules.order Module.symvers *.a
	rm -rf /export/drv/$(TARGET).ko /export/drv/$(TARGET_TEST) /export/drv/$(TARGET_TEST2)