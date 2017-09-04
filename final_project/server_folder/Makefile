KDIR = /lib/modules/$(shell uname -r)/build

obj-m :=sensor.o

default:
	$(MAKE) -C $(KDIR) M=$$PWD modules

clean:
	rm -rf *.o *.ko *.mod.* *.cmd .module* modules* Module* .*.cmd .tmp*
	$(MAKE) -C $(KDIR) M=$$PWD clean
