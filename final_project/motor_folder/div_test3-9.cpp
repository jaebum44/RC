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

#include <omp.h>
#include <wiringPi.h>
#include "pca9685.h"

sem_t motor_sync;
sem_t servo_sync;

using namespace cv;
using namespace std;

#define in1 3
#define in2 5
#define in3 26
#define in4 27

#define PIN_BASE 300
#define MAX_PWM 4096
#define HERTZ 50

#define MAX_SL 3
#define MIN_SL 0.2
#define SP_NR 1250
#define SP_TR 1100

#define PARAM_M1 9>>4
#define PARAM_M2 5>>4
#define PARAM_MSTD 1>>4

#define PARAM_P1 7>>4
#define PARAM_P2 11>>4
#define PARAM_PSTD 15>>4

#define RECV_PACK1 24 
#define RECV_PACK2 25 

#define sc(a,b,c,d) (float)(((float)(d)-(float)(b))\
					/((float)(c)-(float)(a)))

int motor_ctrl(float,float,int);
void*servo_control(void*arg);
void*web_opencv(void*arg);
void*wheel_a(void*arg);

int servo;
int dc_motor;
int DC[ ][ 2 ] = {SP_NR,SP_TR,0,0};

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

	//omp_set_num_threads(3); //쓰레드에 코어 4개를 사용

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
	Mat src, dst1, color_dst1, blur1;

	float*array_sl;
	float*array_val;

	float  sl_min;
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

		Mat image_rot1=src(roi1);
		
		GaussianBlur(image_rot1,blur1,Size(9,9),2.0);
		Canny(blur1,dst1,150,200,3);
		cvtColor(dst1,color_dst1,CV_GRAY2BGR);

		vector<Vec4i> lines;
		HoughLinesP(dst1,lines,1,CV_PI/180,50,30,10);
			
		array_sl=new float[lines.size()];
		array_val=new float[lines.size()];

		#pragma omp parallel for       
		for(int i=0;i<lines.size();i++)
		{

			array_sl[i]= sc(lines[i][0],lines[i][1],lines[i][2],lines[i][3]);
			array_val[i]= (float)( lines[i][1]-array_sl[i]*(lines[i][0]) );
			
			if(!i)
			{
				sl_min= array_sl[i];
				y_val_min= array_val[i];
			}
			
			if(abs(sl_min) > abs(array_sl[i]))
			{
				sl_min= array_sl[i];
				y_val_min= array_val[i];
			}

			line(color_dst1,Point(lines[i][0], lines[i][1]),Point(lines[i][2],lines[i][3]),Scalar(0,0,255),2,5);
		}


		sl_servo = (float)( (-1)*(y_val_min)/sl_min );

		command[0] = digitalRead( RECV_PACK1 );
		command[1] = digitalRead( RECV_PACK2 );
		
		dc_motor = DC[ command[0] ][ command[1] ];

		printf("%.2f\n",sl_min);

		if(abs(sl_min) > MIN_SL && abs(sl_min) < MAX_SL)
		{
			motor_ctrl(sl_servo,sl_min,src.cols);
		}
		else
		{
			//printf("forward\n");
			//servo=240;
			dc_motor*= 0.8;
			sem_post(&servo_sync);
		}

		delete []array_sl;
		delete []array_val;
		
		imshow("src",color_dst1);
		waitKey(1);		
	}
}

int motor_ctrl(float sl_servo, float sl_min, int cols)
{
	//if(abs(sl_min) > MIN_SL && abs(sl_min) < MAX_SL)
	//{
		if( sl_min > 0 && sl_servo < cols*PARAM_PSTD)
		{

			if(sl_servo < cols*PARAM_P1)
			{
				printf("turn left \n");
				servo=165;
				dc_motor*= 1.3;
			}
			else if(sl_servo < cols*PARAM_P2)
			{
				printf("turn little left\n");
				servo=200;
				dc_motor*= 1.2;
			}
			else
			{
				printf("left correction\n");
				servo=230;
			}

			sem_post(&servo_sync);
		}
		else if( sl_min < 0 &&  sl_servo > cols*PARAM_MSTD) 
		{

			if(sl_servo > cols*PARAM_M1)
			{
				servo=315;
				printf("turn right %d\n",servo);
				dc_motor*= 1.3;
			}
			else if(sl_servo > cols*PARAM_M2)
			{
				printf("turn little right \n");
				servo=280;
				dc_motor*= 1.2;
			}
			else
			{
				printf("right correction\n");
				servo=250;
			}

			sem_post(&servo_sync);
		}
		else
		{
			printf("forward\n");
			servo=240;
			dc_motor*= 1.1;
			sem_post(&servo_sync);
		}
	//}

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
		
	while(1) 
	{
		sem_wait(&motor_sync);
		pwmWrite(PIN_BASE+4,dc_motor);
		pwmWrite(PIN_BASE+5,dc_motor);
	}			
}
