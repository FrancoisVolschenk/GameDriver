obj-m += serdev_controller.o

PWD := $(CURDIR)

all: module dt
	echo Built driver and DT overlay
	sudo insmod ./serdev_controller.ko
	sudo chmod 777 /dev/devDriver

module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
dt: serdev_overlay.dts
	dtc -@ -I dts -O dtb -o serdev_overlay.dtbo serdev_overlay.dts
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf serdev_overlay.dtbo

