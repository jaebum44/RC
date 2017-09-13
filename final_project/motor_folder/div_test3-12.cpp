#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>

#include <omp.h>
#include <wiringPi.h>
#include "pca9685.h"

sem_t motor_sync;
sem_t servo_sync;

using namespace cv;
using namespace std;

#define PIN_BASE 300
#define MAX_PWM 4096
#define HERTZ 50

#define MAX_SL 3.5
#define MIN_SL 0.1
#define SP_FAST		1250
#define SP_SLOW		1100
#define SP_STOP		0
#define SP_SLOW_STOP	0

#define PARAM_LEFT1 15>>4
#define PARAM_LEFT2 5>>4
#define PARAM_LEFT3 10>>4

#define PARAM_RIGHT1 1>>4
#define PARAM_RIGHT2 11>>4
#define PARAM_RIGHT3 6>>4


#define in1 4
#define in2 5
#define in3 26
#define in4 27
#define sc(a,b,c,d) (float)(((float)(d)-(float)(b))/((float)(c)-(float)(a)))

#define RECV_PACK1 24 
#define RECV_PACK2 25 

int motor_ctrl(float sl_servo,float sl_min, int cols);
void*servo_control(void*arg);
void*web_opencv(void*arg);
void*wheel_a(void*arg);

int servo;
int dc_motor;
int DC[ 2 ][ 4 ] = {
	SP_FAST, SP_SLOW,      SP_STOP, SP_SLOW_STOP,
	SP_STOP, SP_SLOW_STOP, SP_STOP, SP_SLOW_STOP
};

int main()
{
	wiringPiSetup();

	pinMode( RECV_PACK1, INPUT );
	pinMode( RECV_PACK2, INPUT );

	int fd = pca9685Setup(PIN_BASE, 0x40, HERTZ);
	if (fd < 0)
	{
		printf("Error in setup\n");
		return fd;
	}

	pinMode(in1, OUTPUT);
	pinMode(in2, OUTPUT);
	pinMode(in3, OUTPUT);
	pinMode(in4, OUTPUT);

	pca9685PWMReset(fd);

	pthread_t pid[3];

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
			pthread_create(&pid[1],NULL,servo_control,NULL);	
		}
		#pragma omp section
		{
			pthread_create(&pid[2],NULL,wheel_a,NULL);	
		}
	}
	
	pthread_join(pid[0],NULL);
	pthread_join(pid[1],NULL);
	pthread_join(pid[2],NULL);

	return 0;
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

	VideoCapture cap(0);

	cap.set(CAP_PROP_FRAME_WIDTH,320);
	cap.set(CAP_PROP_FRAME_HEIGHT,240);

	while(1)
	{
		cap>>src;

		Rect roi1(0,src.rows*2/3,src.cols,src.rows/3);

		image_rot1=src(roi1);
		
		GaussianBlur(image_rot1,blur1,Size(9,9),2.0);
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

		addWeighted(image_rot1, 1.0, image_rot1, 0.3, 0, image_rot1);
		line(image_rot1,Point(image_rot1.cols/2,image_rot1.rows),Point(image_rot1.cols/2,image_rot1.rows>>2),Scalar(0,0,0),2,8);


		sl_servo=(float)((-1)*(y_val_min)/sl_min);

		command[0] = digitalRead( RECV_PACK1 );
		command[1] = digitalRead( RECV_PACK2 );
		
		dc_motor = DC[ command[0] ][ command[1] ];

		printf("%.2f\n",sl_min);

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
				servo=160;
				dc_motor*= 1.4;
				printf("turn left %d\n",dc_motor);
			}
			else if(sl_servo < cols*PARAM_LEFT3)
			{
				servo=200;
				dc_motor*= 1.2;
				printf("turn little left %d\n",dc_motor);
			}
			else
			{
				servo=225;
				//dc_motor*=1.1;
				printf("left correction %d\n",dc_motor);
			}

			sem_post(&servo_sync);
		}
		else if( sl_min < 0 &&  sl_servo > cols*PARAM_RIGHT1) 
		{

			if(sl_servo > cols*PARAM_RIGHT2)
			{
				servo=320;
				dc_motor*= 1.4;
				printf("turn right %d\n",dc_motor);
			}
			else if(sl_servo > cols*PARAM_RIGHT3)
			{
				servo=280;
				dc_motor*= 1.2;
				printf("turn little right %d\n",dc_motor);
			}
			else
			{
				servo=255;
				//dc_motor*= 1.1;
				printf("right correction %d\n",dc_motor);
			}
			
			sem_post(&servo_sync);
		}
		else
		{
			printf("forward\n");
			servo=240;
			dc_motor*= 1.0;
			sem_post(&servo_sync);
		}
	}

	return 0;
}

void*servo_control(void*arg) //서보모터 구동부
{

	while(1)
	{
		sem_wait(&servo_sync);
		pwmWrite(PIN_BASE, servo);
		delay(50);
		sem_post(&motor_sync);
	}
}

void*wheel_a(void*arg)  //dc모터 구동부
{
	digitalWrite(in1, LOW);
	digitalWrite(in2, HIGH);
	digitalWrite(in3, HIGH);
	digitalWrite(in4, LOW);
		
	while(1) {
		sem_wait(&motor_sync);
		pwmWrite(PIN_BASE+4,dc_motor);
		//delay(100);
		pwmWrite(PIN_BASE+5,dc_motor);
		//delay(100);
	}			
}
