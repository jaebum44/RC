## compile on 4.9.33 kernel

new_raspberry_source.cpp -> runfile( running on raspberry server )

div_test3-7(1).cpp -> run_div( running on motor control )

>g++ div_test3-7\ \(1\).cpp -o run_dev -lpthread -lwiringPi `pkg-config --cflags --libs opencv` -lwiringPiDev -lwiringPiPca9685 -lm

>g++ new_raspberry_source.cpp -o runfile -lpthread -wiringPi `pkg-config --cflags --libs opencv`
