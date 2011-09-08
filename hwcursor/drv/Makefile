ifneq ($(KERNELRELEASE),)
	obj-m := hwcursor.o
else
default:
	$(MAKE) -C $(KERNELDIR) M=$(shell pwd) ARCH=arm modules
endif
