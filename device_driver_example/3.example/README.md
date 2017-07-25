# make device driver

<http://raspberrypi.ssu.ac.kr/control/2551>

## 모듈 생성

```
cp gpio-morse.c /usr/src/linux/drivers/gpio/
cd /usr/src/linux/
vi driver/gpio/Makefile
```

추가

```
obj-m		+= gpio-morse.o
```

저장 후 종료

## 응용 계층 호출, 모듈 해제, 확인

```
make
insmod driver/gpio/gpio-morse.ko
dmesg
mknod /dev/morse c 230 0
ls /dev/morse -al
cd /home/pi/github/RC/device_driver_example/3.example/
gcc gpio_morse_app.c -o gpio_morse_app
./gpio_morse_app
dmesg
rm /dev/morse
rmmod gpio-morse
lsmod | grep gpio_morse
dmesg
```
