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

using namespace cv;
using namespace std;

#define PWM 1

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
	//pthread_create(&pid[1],NULL,servo_control,NULL);	

	pthread_join(pid[0],NULL);
	//pthread_join(pid[1],NULL);

	return 0;
}

void*web_opencv(void*arg)
{
	Mat src, dst1, dst2, color_dst1, color_dst2, blur1, blur2;

	VideoCapture cap(0);

	while(1)
	{
		cap>>src;
		
		Rect roi(1,1,639,300);

		Rect roi1(0,src.rows/2,src.cols,src.rows/2);
		Mat image_rot1=src(roi1);
		
		GaussianBlur(image_rot1,blur1,Size(9,9),2.0);
		erode(blur1,blur1,Mat(),Point(-1,-1));
		dilate(blur1,blur1,Mat(),Point(-1,-1));
		Canny(blur1,dst1,150,200,3);
		cvtColor(dst1,color_dst1,CV_GRAY2BGR);

		vector<Vec4i> lines;
		HoughLinesP(dst1,lines,1,CV_PI/180,50,30,10);
		for(int i=0;i<lines.size();i++)
		{
			float rl=(float)(lines[i][3])-(float)(lines[i][1]);
			float cl=(float)(lines[i][2])-(float)(lines[i][0]);

			if((float)(cl/rl)*(-1) > -1.7 && (float)(cl/rl)*(-1) < -0.3)
			{
				line(color_dst1,Point(lines[i][0], lines[i][1]),Point(lines[i][2],lines[i][3]),Scalar(0,0,255),2,5);
				printf("%f\n",cl/rl);
				if(cl/rl > 1.2)
					printf("Turn Left\n");
			} 

			if((float)(cl/rl)*(-1) > 0.3 && (float)(cl/rl)*(-1) < 1.7)
			{
				line(color_dst1,Point(lines[i][0], lines[i][1]),Point(lines[i][2],lines[i][3]),Scalar(0,255,255),2,5);
				//printf("%f\n",cl/rl);
				if(cl/rl < -1.07)
					printf("Turn Right\n");
			} 

		}

		imshow("window label1",color_dst1);
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
		if(mx.m_servo == 0)
		{
			softPwmWrite(PWM, 13); 
			delay(200);
			softPwmWrite(PWM, 27); 
			delay(200);
		}
		else if(mx.m_servo == 1)
		{	
			softPwmWrite(PWM, 20); 
			delay(200);
		}
	}
}
