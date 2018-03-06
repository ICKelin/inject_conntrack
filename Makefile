module_name := inject_conntrack.ko 
obj-m := inject_conntrack.o 
kernel_path   :=/lib/modules/$(shell uname -r)/build

all: 
	make -C $(kernel_path) M=$(PWD) modules

install:
	insmod $(module_name) 

uninstall:
	rmmod $(obj-m)

clean:
	rm -rf *.o *.mod.c *.ko
