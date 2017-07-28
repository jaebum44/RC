# LED device driver

Move gpioled.c to linux/driver/gpio

running command lines after input 'make' command on linux folder

```
insmod gpioled.ko
mknod /dev/gpioled c 200 0
```
