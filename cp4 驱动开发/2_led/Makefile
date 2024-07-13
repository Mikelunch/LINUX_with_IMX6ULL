export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-

KERNELDIR := /home/constant_z/linux/IMX6ULL/linux/alientek_linux
POJECT_NAME := led
CURRENT_PATH := $(shell pwd)

obj-m := $(POJECT_NAME).o

build: kernel_modules
	@echo "copy to rootfs"
	sudo cp $(POJECT_NAME).ko /home/constant_z/linux/nfs/rootfs/lib/modules/4.1.15/ -f

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules

buildApp: makeApp
	@echo "make "$(POJECT_NAME)"App"
	@echo "copy to rootfs"
	sudo cp $(POJECT_NAME)App /home/constant_z/linux/nfs/rootfs/lib/modules/4.1.15/ -f

makeApp:
	arm-linux-gnueabihf-gcc $(POJECT_NAME)App.c -o $(POJECT_NAME)App

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
