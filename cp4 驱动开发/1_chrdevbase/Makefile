export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-

KERNELDIR := /home/constant_z/linux/IMX6ULL/linux/alientek_linux

CURRENT_PATH := $(shell pwd)

obj-m := chrdevbase.o

build: kernel_modules
	@echo "copy to rootfs"
	sudo cp chrdevbase.ko /home/constant_z/linux/nfs/rootfs/lib/modules/4.1.15/ -f

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules

buildApp: makeApp
	@echo "copy to rootfs"
	sudo cp chardevApp /home/constant_z/linux/nfs/rootfs/lib/modules/4.1.15/ -f

makeApp:
	arm-linux-gnueabihf-gcc chardevApp.c -o chardevApp

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
