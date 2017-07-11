#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>

#include <stdio.h>
#include <wiringPi.h>
#include <softPwm.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>

using namespace cv;
using namespace std;

#define PWM 1
#define sc(a,b,c,d) fabs((float)(((float)(c)-(float)(a))/((float)(d)-(float)(b))))

typedef struct x
{
	char m_servo;

}devControl;

devControl mx;

void*servo_control(void*arg);
void*web_opencv(void*arg);

int main()
{

	pthread_t pid[2];

	wiringPiSetup();
	
	pthread_create(&pid[0],NULL,web_opencv,NULL);
	pthread_create(&pid[1],NULL,servo_control,NULL);	

	pthread_join(pid[0],NULL);
	pthread_join(pid[1],NULL);

	return 0;
}

void*web_opencv(void*arg)
{
	Mat src, dst1, dst2, color_dst1, color_dst2, blur1, blur2;

	float sl;

	VideoCapture cap(0);

	while(1)
	{
		cap>>src;

		Rect roi1(src.cols/2,src.rows/2,src.cols/2,src.rows/2);
		Rect roi2(0,src.rows/2,src.cols/2,src.rows/2);

		Mat image_rot1=src(roi1);
		Mat image_rot2=src(roi2);
		
		GaussianBlur(image_rot1,blur1,Size(9,9),2.0);
		GaussianBlur(image_rot2,blur2,Size(9,9),2.0);
		erode(blur1,blur1,Mat(),Point(-1,-1));
		dilate(blur1,blur1,Mat(),Point(-1,-1));
		erode(blur2,blur2,Mat(),Point(-1,-1));
		dilate(blur2,blur2,Mat(),Point(-1,-1));
		Canny(blur1,dst1,100,200,3);
		Canny(blur2,dst2,100,200,3);
		cvtColor(dst1,color_dst1,CV_GRAY2BGR);
		cvtColor(dst2,color_dst2,CV_GRAY2BGR);

		vector<Vec4i> lines;
		HoughLinesP(dst1,lines,1,CV_PI/180,30,30,10);
		for(int i=lines.size();i>0;i--)
		{
			sl=sc(lines[i][0],lines[i][1],lines[i][2],lines[i][3]);

			switch((sl*10)/12)
			{
			case 1:
				printf("Turn left\n");
				mx.m_servo = 1;
				line(color_dst1,Point(lines[i][0], lines[i][1]),Point(lines[i][2],lines[i][3]),Scalar(0,0,255),2,5);
				break;
			default:
				printf("forward\n");
				mx.m_servo = 0;
				line(color_dst1,Point(lines[i][0], lines[i][1]),Point(lines[i][2],lines[i][3]),Scalar(0,0,255),2,5);
				break;
			}
		}

		vector<Vec4i> lines2;
		HoughLinesP(dst2,lines2,1,CV_PI/180,30,30,10);

		for(int i=lines.size();i>0;i--)
		{
			sl=sc(lines[i][0],lines[i][1],lines[i][2],lines[i][3]);

			switch((sl*10)/10)
			{
			case 1:
				printf("Turn right\n");
				mx.m_servo = 2;
				line(color_dst1,Point(lines[i][0], lines[i][1]),Point(lines[i][2],lines[i][3]),Scalar(0,0,255),2,5);
				break;
			default:
				printf("forward\n");
				mx.m_servo = 0;
				line(color_dst1,Point(lines[i][0], lines[i][1]),Point(lines[i][2],lines[i][3]),Scalar(0,0,255),2,5);
				break;
			}
		}
		imshow("window label1",color_dst1);
		imshow("window label2",color_dst2);
		waitKey(1);
	}
}

	

void*servo_control(void*arg)
{

	wiringPiSetup();

	pinMode(PWM, OUTPUT);
	
	softPwmCreate(PWM, 0, 300);

	while(1)
	{
		switch(mx.m_servo)
		{
		case 0:
			softPwmWrite(PWM, 19.5);
			delay(200);
			break;
		case 1:
			softPwmWrite(PWM, 14);
			delay(200);			
			break;
		case 2:	
			softPwmWrite(PWM, 24);
			delay(200);
			break;
		}
	}
}
