# Default value for USE_VM
USE_VM ?= 0

ifeq ($(USE_VM),0)
    KERNELDIR := /home/reds/linux-socfpga/
    TOOLCHAIN := /opt/toolchains/arm-linux-gnueabihf_6.4.1/bin/arm-linux-gnueabihf-
else
    KERNELDIR := /home/rafou/Desktop/heig-vd/DRV/linux-socfpga/
    TOOLCHAIN := /home/rafou/Desktop/heig-vd/DRV/gcc-linaro-6.4.1-2018.05-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
endif

obj-m := exercice_chrono.o

GCC := $(TOOLCHAIN)gcc

PWD := $(shell pwd)
WARN := -W -Wall -Wstrict-prototypes -Wmissing-prototypes

TARGET := exercice_chrono
TARGET_TEST := test_ex_chrono

all: clean $(TARGET) $(TARGET_TEST) deploy

$(TARGET):
	@echo "Building with kernel sources in $(KERNELDIR)"
	$(MAKE) ARCH=arm CROSS_COMPILE=$(TOOLCHAIN) -C $(KERNELDIR) M=$(PWD) ${WARN}
	rm -rf *.o *~ core .depend .*.cmd *.mod *.mod.c .tmp_versions modules.order Module.symvers *.a

$(TARGET_TEST):
	$(GCC) $@.c -o $@

deploy:
	cp $(TARGET).ko $(TARGET_TEST)  /export/drv
clean:
	rm -rf $(TARGET).ko $(TARGET_TEST) *.o *~ core .depend .*.cmd *.ko *.mod *.mod.c .tmp_versions modules.order Module.symvers *.a
	rm -rf /export/drv/$(TARGET).ko /export/drv/$(TARGET_TEST)