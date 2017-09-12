obj-m := sample.o print_int.o

KDIR := /usr/src/linux
# KDIR := /lib/modules/$(shell uname -r)/build

PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	rm -f *.ko
	rm -f *.o
	rm -f *.mod*
	rm -f *.cmd
	rm -f *.order
	rm -f *.symvers
	rm -rf .*
