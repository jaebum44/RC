#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <iostream>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include "pca9685.h"
#include <wiringPi.h>
#include <omp.h>

sem_t pread_sync;
sem_t pwrite_sync;
sem_t motor_sync;
sem_t servo_sync;
sem_t opencv_sync;

using namespace cv;
using namespace std;

#define PIN_BASE 		300
#define MAX_PWM 		4096
#define HERTZ 			50

#define MAX_SL			3.5
#define MIN_SL 			0.1
#define SP_FAST			1500
#define SP_SLOW			1300
#define SP_STOP			0
#define SP_SLOW_STOP		0

#define PARAM_LEFT1 	14>>4
#define PARAM_LEFT2 	5>>4
#define PARAM_LEFT3 	10>>4

#define PARAM_RIGHT1 	2>>4
#define PARAM_RIGHT2 	11>>4
#define PARAM_RIGHT3 	6>>4

#define filename 		"/dev/i2c-1"
#define MODE1 			0x00
#define MODE2 			0x01
#define PRE_SCALE		 0xFE
#define CLOCK_FREQ 		25000000.0
#define CHANNEL0_ON_L		0x06
#define CHANNEL0_ON_H		0x07
#define CHANNEL0_OFF_L		0x08
#define CHANNEL0_OFF_H		0x09
#define CHANNEL4_ON_L		0x16
#define CHANNEL4_ON_H		0x17
#define CHANNEL4_OFF_L		0x18
#define CHANNEL4_OFF_H		0x19
#define CHANNEL5_ON_L		0x1A
#define CHANNEL5_ON_H		0x1B
#define CHANNEL5_OFF_L		0x1C
#define CHANNEL5_OFF_H		0x1D
#define ALL_LED_ON_L 		0xFA

#define in1			4
#define in2 			5
#define in3 			26
#define in4 			27

#define RECV_PACK1 		24 
#define RECV_PACK2 		25 

#define sc(a,b,c,d) (float)(((float)(d)-(float)(b))/((float)(c)-(float)(a)))

int motor_ctrl(float sl_servo,float sl_min, int cols);
void*servo_control(void*arg);
void*web_opencv(void*arg);
void*wheel_a(void*arg);
int init_motor(void);

int pca9685_reset(int fd);
int fileopen(void);
int led_on(int fd);
int pca9685_freq(int fd);
int reg_write16(int addr, int data, int fd_pca);
int reg_read16(int addr,int fd_pca);
void pca_channel4and5_on (int*fd);
void pca_servo_on0(int time_val_on,int time_val_on1,int*fd);

int servo;
int dc_motor;

int pca_addr = 0x40;

int DC[ 2 ][ 4 ] = {
	SP_FAST, SP_SLOW,      SP_STOP, SP_SLOW_STOP,
	SP_STOP, SP_SLOW_STOP, SP_STOP, SP_SLOW_STOP
};

int main()
{	
	int fd_pca;
	init_motor();

	pthread_t pid[3];
	
	fd_pca =fileopen();
	//omp_set_num_threads(4); //쓰레드에 코어 4개를 사용

	#pragma omp parallel //openmp시작
	#pragma omp sections //openmp로 섹션을 나누어서 한섹션에 1코어를 할당
	{
		#pragma omp section
		{
			pthread_create(&pid[0],NULL,web_opencv,NULL);
		}
		#pragma omp section
		{
			pthread_create(&pid[1],NULL,servo_control, (void *)&fd_pca);	
		}
		#pragma omp section
		{
			pthread_create(&pid[2],NULL,wheel_a,(void *)&fd_pca);	
		}
	}
	
	pthread_join(pid[0],NULL);
	pthread_join(pid[1],NULL);
	pthread_join(pid[2],NULL);

	return 0;
}

int init_motor()
{
	int init=1;
	unsigned int max_write =5;
		wiringPiSetup();

		pinMode( RECV_PACK1, INPUT );
		pinMode( RECV_PACK2, INPUT );

		sem_init(&pwrite_sync , 1 ,5);
		sem_init(&pread_sync , 0 ,5);
		sem_init(&opencv_sync, 1,5);
		
		pinMode(in1, OUTPUT);
		pinMode(in2, OUTPUT);
		pinMode(in3, OUTPUT);
		pinMode(in4, OUTPUT);

		digitalWrite(in1, LOW);
		digitalWrite(in2, HIGH);
		digitalWrite(in3, HIGH);
		digitalWrite(in4, LOW);
}

void*web_opencv(void*arg) 
{
	Mat src, dst1, blur1, image_rot1;

	float*array_sl;
	float*array_val;

	float sl_min;
	float y_val_min;
	float sl_servo;

	int command[2];
	sem_wait(&opencv_sync);
	VideoCapture cap(0);

	cap.set(CAP_PROP_FRAME_WIDTH,320);
	cap.set(CAP_PROP_FRAME_HEIGHT,240);

	while(1)
	{
		cap>>src;

		Rect roi1(0,src.rows*2/3,src.cols,src.rows/3);

		image_rot1=src(roi1);
		
		GaussianBlur(image_rot1,blur1,Size(9,9),2);
		Canny(blur1,dst1,150,200,3);

		vector<Vec4i> lines;
		HoughLinesP(dst1,lines,1,CV_PI/180,50,50,10);
			
		array_sl=new float[lines.size()];
		array_val=new float[lines.size()];

		#pragma omp parallel for       
		for(int i=0;i<lines.size();i++)
		{

			array_sl[i]=sc(lines[i][0],lines[i][1],lines[i][2],lines[i][3]);
			array_val[i]=(float)(lines[i][1]-array_sl[i]*(lines[i][0]));
			
			if(!i)
			{
				sl_min=array_sl[i];
				y_val_min=array_val[i];
			}
			
			if(abs(sl_min) > abs(array_sl[i]))
			{
				sl_min=array_sl[i];
				y_val_min=array_val[i];
			}

			line(image_rot1,Point(lines[i][0], lines[i][1]),Point(lines[i][2],lines[i][3]),Scalar(255,0,0),2,8);
		}

		//addWeighted(image_rot1, 1.0, image_rot1, 1.0, 0, image_rot1);
		line(image_rot1,Point(image_rot1.cols/2,image_rot1.rows),Point(image_rot1.cols/2,image_rot1.rows>>2),Scalar(0,0,0),2,8);


		sl_servo=(float)((-1)*(y_val_min)/sl_min);

		command[0] = digitalRead( RECV_PACK1 );
		command[1] = digitalRead( RECV_PACK2 );
		
		dc_motor = DC[ command[0] ][ command[1] ];

		printf("%d\n", dc_motor);

		printf("command =  %d %d\n", command[0],  command[1]);
		motor_ctrl(sl_servo,sl_min,src.cols);

		delete []array_sl;
		delete []array_val;
		
		imshow("detected lines",src);
		waitKey(1);		
	}
}

int motor_ctrl(float sl_servo, float sl_min, int cols)
{
	
	if(abs(sl_min) < MAX_SL && abs(sl_min) > MIN_SL)
	{
		if( sl_min > 0 && sl_servo < cols*PARAM_LEFT1)
		{

			if(sl_servo < cols*PARAM_LEFT2)
			{
				servo = 350;
				dc_motor *= 1.3;
				printf("turn left %d\n",dc_motor);
			}
			else if(sl_servo < cols*PARAM_LEFT3)
			{
				servo = 400;
				dc_motor *= 1.2;
				printf("turn little left %d\n",dc_motor);
			}
			else
			{
				servo = 450;
				printf("left correction %d\n",dc_motor);
			}

			sem_post(&servo_sync);
		}
		else if( sl_min < 0 &&  sl_servo > cols*PARAM_RIGHT1) 
		{

			if(sl_servo > cols*PARAM_RIGHT2)
			{
				servo = 650;
				dc_motor *= 1.3;
				printf("turn right %d\n",dc_motor);
			}
			else if(sl_servo > cols*PARAM_RIGHT3)
			{
				servo = 600;
				dc_motor *= 1.2 ;
				printf("turn little right %d\n",dc_motor);
			}
			else
			{
				servo=520;
				printf("right correction %d\n",dc_motor);
			}				
			
			sem_post(&servo_sync);
		}		
		else
		{
			printf("forward\n");
			servo=500;
			dc_motor *= 1.2 ;
			sem_post(&servo_sync);
		}
	}

	return 0;
}

void* servo_control(void*arg) //서보모터 구동부
{
	int fd_pca = *(int*)arg;

	while(1)
	{
		sem_wait(&servo_sync);
		pca_servo_on0(1000 ,servo, &fd_pca);//50 맨왼쪽으로 회전
		delay(50);
		sem_post(&motor_sync);
	}
}


void* wheel_a(void*arg)  //dc모터 구동부
{
	int fd_pca =*(int*)arg;

	while(1) 
	{
		
		sem_wait(&motor_sync);
		pca_channel4and5_on(&fd_pca);
	}			
}

void pca_servo_on0(int time_val_on, int time_val_on1, int*fd)
{
	int*fd_pca;

	*fd_pca = *fd;

	reg_write16(CHANNEL0_ON_L, time_val_on, *fd_pca);
	reg_read16(CHANNEL0_ON_L, *fd_pca);
		
	reg_write16(CHANNEL0_OFF_L, time_val_on + time_val_on1, *fd_pca);
	reg_read16(CHANNEL0_OFF_L,*fd_pca);

}	
	
void pca_channel4and5_on(int*fd)
{
	int*fd_pca;
	*fd_pca = *fd;
	int time_val_on = 0;
	char key;
	
	reg_write16(CHANNEL4_ON_L, 0, *fd_pca);
	reg_read16(CHANNEL4_ON_L, *fd_pca);

	reg_write16(CHANNEL4_OFF_L, dc_motor, *fd_pca);
	reg_read16(CHANNEL4_OFF_L, *fd_pca);

	reg_write16(CHANNEL5_ON_L, 0,*fd_pca);
	reg_read16(CHANNEL5_ON_L,*fd_pca);

	reg_write16(CHANNEL5_OFF_L, dc_motor, *fd_pca);
	reg_read16(CHANNEL5_OFF_L,*fd_pca);

}
	
int pca9685_freq(int fd)
{
	int length = 2, freq = 100, pca_addr = 0x40;
	unsigned char buffer[60] = {0};
	uint8_t prescale_val = (CLOCK_FREQ / 4096 / freq) -1;
	printf("prescale_val = %d \n", prescale_val);
	if(ioctl(fd,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}
	buffer[0] = MODE1;
	buffer[1] = 0x10;
	if(write(fd,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}
	
	if(ioctl(fd,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}
	buffer[0] = PRE_SCALE;
	buffer[1] = prescale_val;
	if(write(fd,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}
	
	if(ioctl(fd,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}
	buffer[0] = MODE1;
	buffer[1] = 0x80;
	if(write(fd,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}
	else{
			printf("Data Mode1 %x\n",buffer[0]);
	}

	if(ioctl(fd,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}
	buffer[0] = MODE2;
	buffer[1] = 0x04;
	length = 2;
	if(write(fd,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}
	else{
			printf("Data Mode2 %x\n",buffer[0]);
	}
}

int pca9685_reset(int fd)
{
	unsigned char buffer[60] = {0};
	int length, pca_addr = 0x40;
	
	if(ioctl(fd,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}
	buffer[0] = MODE1;
	buffer[1] = 0x00;
	length = 2;
	if(write(fd,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}
	else{
			printf("Data Mode1 %x\n",buffer[0]);
	}

	if(ioctl(fd,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}
	buffer[0] = MODE2;
	buffer[1] = 0x04;
	length = 2;
	if(write(fd,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}
	else{
		printf("Data Mode2 %x\n",buffer[0]);
	}
}


int reg_read16(int addr,int fd_pca)
{
	
	unsigned char buffer[60] = {0};
	int temp = 0 , length = 0;
	// reg addr write 
	if(ioctl(fd_pca,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}
	buffer[0] = addr;
	length =1;
	if(write(fd_pca,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}
	// reg read 
	if(ioctl(fd_pca,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}
	
	if(read(fd_pca,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}
	temp = buffer[0];
		// reg addr write 
	if(ioctl(fd_pca,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}
	
	buffer[0] = addr + 1;
	if(write(fd_pca,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}
	// reg read 
	if(ioctl(fd_pca,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}
	
	if(read(fd_pca,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}
	temp = buffer[0]<<8 | temp;
//	printf("addr[%d] = %d\n",addr,temp);
}

int reg_write16(int addr, int data, int fd_pca)
{
	unsigned char buffer[60] = {0};
	int length =2;
	if(ioctl(fd_pca,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}

	buffer[0] = addr;
	buffer[1] = data & 0xff;

	
	if(write(fd_pca,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}	

	if(ioctl(fd_pca,I2C_SLAVE,pca_addr)<0){
		printf("Failed to acquire bus access and/or talk to slave\n");
		return 0;
	}

	buffer[0] = addr+1;
	buffer[1] = (data>>8) & 0xff;

	if(write(fd_pca,buffer,length) != length){
		printf("Failed to write from the i2c bus\n");
		return 0;
	}	
	//printf("addr[%d]= %d\n",addr,data);
}

int fileopen(void)
{
	int fd_pca;
	if((fd_pca = open(filename, O_RDWR))<0){
		printf("Failed to open the i2c bus\n");
		return -1;
	}
	pca9685_reset(fd_pca);
	pca9685_freq(fd_pca);
	
	return fd_pca;	
}




