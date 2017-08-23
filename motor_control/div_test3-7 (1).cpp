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

#define in1 4
#define in2 5
#define in3 26
#define in4 27
#define sc(a,b,c,d) (float)(((float)(d)-(float)(b))/((float)(c)-(float)(a)))

#define RECV_PACK1 24 
#define RECV_PACK2 25 

#define DEBUG_MODE 0

typedef int(*FCMP)(const void*, const void*);

float servo;
float dc_motor;
char for_exit;

void*servo_control(void*arg);
void*web_opencv(void*arg);
void*wheel_a(void*arg);
void*kill_process(void*arg);

void* recv_pack( void* );

float DC[ ][ 2 ] = \
	{ 1500	, 1100,
	  0	, 0	};

void* recv_pack( void* arg )
{
	while( 1 )
	{
		dc_motor = DC[ digitalRead( RECV_PACK1 ) ][ digitalRead( RECV_PACK2 ) ];
		printf("%d %d\n", digitalRead( RECV_PACK1 ), digitalRead( RECV_PACK2 ));
		sleep(1000);
	}
}

int cmp_f(const void*p, const void*k)
{
	return (int)(*(int*)p-*(int*)k);
}

void*func(void*base,size_t num,size_t width, FCMP fcmp)
{
	fcmp((char*)base+width*(0),(char*)base+width*(1));

	qsort((void*)base,num,width,fcmp);
}


int get_Median(float*array, size_t arraySize)
{
	size_t center=arraySize/2; 

	if (arraySize % 2 == 1)
	{
		return center; 
	} 
	else
	{
		return (2*center-1)/2; 
	}
}

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

	pthread_t pid[4];
	pthread_t main_thrd;

	pthread_create(&main_thrd,NULL,recv_pack,NULL);

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
		#pragma omp section
		{
			pthread_create(&pid[3],NULL,kill_process,NULL);	
		}
	}
	
	pthread_join(pid[0],NULL);
	pthread_join(pid[1],NULL);
	pthread_join(pid[2],NULL);
	pthread_join(pid[3],NULL);
	pthread_join(main_thrd,NULL);

	return 0;
}
void*kill_process(void*arg)
{
	while(1)
	{
		scanf("%c",&for_exit);
	
		if(for_exit == 'c')
		{
			pwmWrite(PIN_BASE+4,0);
			//delay(50);
			pwmWrite(PIN_BASE+5,0);
			//delay(50);
			//pwmWrite(PIN_BASE,240);
			//delay(50);
			exit(0);
		}
	}	
	 
}
void*web_opencv(void*arg) 
{
	Mat src, dst1, color_dst1, blur1;

	float sl[10];
	float  sl_min;
	float y_val[10];
	float y_val_min;
	int j=0;

	float sl_theta[5];
	float sl_theta_avg=0;
	int sl_theta_mid;


	float sl_servo;

	//Point mask_points[1][6];

	//int lineType=8;

	VideoCapture cap(0);

	cap.set(CAP_PROP_FRAME_WIDTH,320);
	cap.set(CAP_PROP_FRAME_HEIGHT,240);

	while(1)
	{
		cap>>src;

		Rect roi1(0,src.rows/2,src.cols,src.rows/2);

		Mat image_rot1=src(roi1);
		
		GaussianBlur(image_rot1,blur1,Size(9,9),2.0);
		//erode(blur1,blur1,Mat(),Point(-1,-1));
		//dilate(blur1,blur1,Mat(),Point(-1,-1));
		Canny(blur1,dst1,150,200,3);
		cvtColor(dst1,color_dst1,CV_GRAY2BGR);

	//	mask_points[0][0]=Point(0,0);
	//	mask_points[0][5]=Point(dst1.cols,0);
	//	mask_points[0][2]=Point(dst1.cols/4,dst1.rows/2);
	//	mask_points[0][3]=Point(dst1.cols*3/4,dst1.rows/2);
	//	mask_points[0][1]=Point(0,dst1.rows);
	//	mask_points[0][4]=Point(dst1.cols,dst1.rows);

	//	const Point*ppt[1]={mask_points[0]};
	//	int npt[]={6};
	//	fillPoly(dst1,ppt,npt,1,Scalar(0,0,0),lineType);

		
		vector<Vec4i> lines;
		HoughLinesP(dst1,lines,1,CV_PI/180,50,50,10);

		#pragma omp parallel for       
		for(int i=0;i<lines.size();i++)
		{
			sl[i]=sc(lines[i][0],lines[i][1],lines[i][2],lines[i][3]);
			y_val[i]=(float)(lines[i][1]-sl[i]*(lines[i][0]));
	
			if(!i)
			{
				sl_min=sl[i];
				y_val_min=y_val[i];
			}
			
			if(abs(sl_min) > abs(sl[i]))
			{
				sl_min=sl[i];
				y_val_min=y_val[i];
			}

			//printf("%d %d\n",lines[i][1],lines[i][0]);
			//printf("%d %d\n",lines[i][3],lines[i][2]);
			
			//printf("%.2f %.2f\n",sl, y_val);
			line(color_dst1,Point(lines[i][0], lines[i][1]),Point(lines[i][2],lines[i][3]),Scalar(0,0,255),2,5);
		}

		//sl_theta[j]	=90-(180/3.1415f*atan2(src.cols/2-(float)((-1)*(y_val)/sl_avg),sl*src.cols/2+y_val));

		//sl_theta_avg	+= sl_theta[j];

		//line(color_dst1,Point(color_dst1.cols/2,color_dst1.rows),Point(color_dst1.cols/2+1,sl*(color_dst1.cols/2+1)+y_val),Scalar(255,0,255),2,5);

		//line(color_dst1,Point(color_dst1.cols/2,color_dst1.rows),Point(color_dst1.cols/2,color_dst1.rows*4/6),Scalar(255,255,0),2,5);

		sl_servo=(float)((-1)*(y_val_min)/sl_min);
		
		//if(j==1)
		//{

			/*if(!lines.size())
			{
				pwmWrite(PIN_BASE+4,950);
				pwmWrite(PIN_BASE+5,950);
			}*/

			if( sl_min > 0 && sl_servo < src.cols*13/16)
			//if( sl_min > 0 && sl_servo < src.cols/2)	
			{
				if(sl_servo < src.cols*11/16)
				{
					printf("turn left \n");
					servo=170;
					//dc_motor=recv_pack();
				}

				//printf("%.2f\n",(float)((-1)*(y_val)/sl));
				else
				{
				//sl_theta_length=sizeof(sl_theta)/sizeof(sl_theta[0]);
				//func(sl_theta,sl_theta_length,sizeof(sl_theta[0]),cmp_f);
				//sl_theta_mid=sl_theta[get_Median(sl_theta,sl_theta_length)];
				//sl_theta_avg=(float)sl_theta_avg/5;
				//printf("theta = %.2f\n",sl_theta_avg);
				//sl_theta_avg=0;
				//if(servo<24.5)
				//	servo+=1.5;
				//else
					printf("turn little left\n");
					servo=210;
					//dc_motor=recv_pack();
				}

				sem_post(&servo_sync);
			}
			else if( sl_min < 0 &&  sl_servo > src.cols*3/16) 
			//else if( sl_min < 0 &&  sl_servo > src.cols/2) 
			{

				if(sl_servo > src.cols*5/16)
				{
					printf("turn right \n");
					servo=310;
					//dc_motor=recv_pack();
				}
				else
				{
					printf("turn little right \n");
					servo=270;
					//dc_motor=recv_pack();
				}
				
				sem_post(&servo_sync);
			}
			else
			{
				printf("forward\n");
				servo=240;
				//dc_motor=recv_pack();
				sem_post(&servo_sync);
			}
		//	j=0;
		//}
		//else
		//	j++;
		imshow("src",color_dst1);
		waitKey(1);		

	}



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
