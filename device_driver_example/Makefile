obj-m := example.o

KDIR := /usr/src/linux-headers-4.8.0-58-generic/

PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	rm -f *.ko
	rm -f *.o
	rm -f *.mod.*
	rm -f .*.cmd
	rm -f *.order
	rm -f *.symvers
