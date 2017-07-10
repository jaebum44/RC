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
#define WHEEL_SET 19.5

float servo = 0;

void* servo_control(void*);
void* web_opencv(void*);
void* cam_thread(void*);
void* line_left(void*);
void* line_right(void*);

int main()
{

	pthread_t pid[2];

	pthread_create(
        &pid[0],
        NULL,
        web_opencv,
        NULL
    );

	pthread_create(
        &pid[1],
        NULL,
        servo_control,
        NULL
    );	

	pthread_join(
        pid[0],
        NULL
    );

	pthread_join(
        pid[1],
        NULL
    );

	return 0;
}

Mat src, dst[2], color_dst[2], blur[2];
int positionRectY[2];
vector<Vec4i> lines[2];
float saveServo[2];

void* web_opencv(void* arg)
{
	pthread_t pid[4];
    int idx0 = 0, idx1 = 1;


	VideoCapture cap(0);

	while(1)
	{
		cap>>src;

        positionRectY[0] = 0;
        positionRectY[1] = src.rows/2;

    	pthread_create(
            &pid[0],
            NULL,
            cam_thread,
            (void*)&idx0
        );
    
    	pthread_create(
            &pid[1],
            NULL,
            cam_thread,
            (void*)&idx1
        );
    
    	pthread_join(
            pid[0],
            NULL
        );
    
    	pthread_join(
            pid[1],
            NULL
        );

    	pthread_create(
            &pid[2],
            NULL,
            line_left,
            (void*)&idx1
        );

    	pthread_create(
            &pid[3],
            NULL,
            line_right,
            (void*)&idx0
        );

    	pthread_join(
            pid[2],
            NULL
        );
    
    	pthread_join(
            pid[3],
            NULL
        );

		imshow(
            "window label1",
            color_dst[1]
        );

		imshow(
            "window label0",
            color_dst[0]
        );
		waitKey(1);
	}
}

void*servo_control(void*arg)
{

	wiringPiSetup();

	pinMode(PWM, OUTPUT);
	
	softPwmCreate(
        PWM,
        0, 300
    );

	while(1)
	{
		softPwmWrite(
            PWM,
            servo
        );
		delay(200);
	}
}

void* cam_thread(void* value)
{
    int idx = *((int*)value);

    Rect roi(
        0, 
        positionRectY[idx],
        src.cols,
        src.rows/2
    );

    Mat image_rot = \
        src(roi);

    GaussianBlur( // reduce line detect
        image_rot,
        blur[idx],
        Size(9, 9),
        2, 0
    );

    erode(
        blur[idx],
        blur[idx],
        Mat(),
        Point(-1, -1)
    );

    dilate(
        blur[idx],
        blur[idx],
        Mat(),
        Point(-1, -1)
    );

/* reason of used erode, dilate function: \
    http://hongkwan.blogspot.kr/2013/01/opencv-5-1-example.html */

    Canny(
        blur[idx],
        dst[idx],
        150, 200,
        3
    );

    cvtColor(
        dst[idx],
        color_dst[idx],
        CV_GRAY2BGR
    );

    HoughLinesP(
        dst[idx],
        lines[idx],
        1,
        CV_PI/180,
        50, 30,
        10
    ); // http://docs.opencv.org/2.4/modules/imgproc/doc/feature_detection.html?highlight=houghlinesp#void HoughLinesP(InputArray image, OutputArray lines, double rho, double theta, int threshold, double minLineLength, double maxLineGap)
}

void* line_left(void* value)
{
    float rl, cl, CLdivRL;
    int idx = *((int*)value);

	for(int i = 0; i < lines[idx].size(); i++)
	{
		rl = \
            (float)(lines[idx][i][3]) - (float)(lines[idx][i][1]);
		cl = \
            (float)(lines[idx][i][2]) - (float)(lines[idx][i][0]);
        CLdivRL = cl/rl;

		if(CLdivRL > -1.5 && CLdivRL < -0.5)
		{
            line(
                color_dst[idx],
                Point(
                    lines[idx][i][0],
                    lines[idx][i][1]
                ),
                Point(
                    lines[idx][i][2],
                    lines[idx][i][3]
                ),
                Scalar(
                    0, 255,
                    255
                ),
                2, 5
            );

    		if(CLdivRL > 1.2) {
    			printf("L: Turn Left\n");
    
    			saveServo[idx] = \
                    WHEEL_SET - ((CLdivRL - 1) * 5);
    
    			printf(
                    "servo = %f\n",
                    saveServo[idx]
                );

    			delay(100);
    		}
    		else if(CLdivRL < 0.8) {
    			printf("L: Turn right\n");
    
    			saveServo[idx] = \
                    WHEEL_SET - ((CLdivRL - 1) * 5);
    
    			printf(
                    "servo = %f\n",
                    saveServo[idx]
                );

    			delay(100);
    		}
			else 
            {
				printf("Straight\n");
		    	saveServo = WHEEL_SET; 
				delay(100);
			}
		}
		else 
        {
			printf("Break\n");
	    	saveServo = WHEEL_SET; 
			delay(100);
	    }
    }
}

void* line_right(void* value)
{
    float rl, cl, CLdivRL;
    int idx = *((int*)value);

	for(int i = 0; i < lines[idx].size(); i++)
	{
		rl = \
            (float)(lines[idx][i][3]) - (float)(lines[idx][i][1]);
		cl = \
            (float)(lines[idx][i][2]) - (float)(lines[idx][i][0]);
        CLdivRL = cl/rl; 

		if(CLdivRL > -1.5 && CLdivRL < -0.5)
		{
            line(
                color_dst[idx],
                Point(
                    lines[idx][i][0],
                    lines[idx][i][1]
                ),
                Point(
                    lines[idx][i][2],
                    lines[idx][i][3]
                ),
                Scalar(
                    0, 255,
                    255
                ),
                2, 5
            );

			if(CLdivRL < -1.2) {
				printf("R: Turn Right\n");

				saveServo[idx] = \
                    WHEEL_SET+((CLdivRL - 1) * 5);

				printf(
                    "servo = %f\n",
                    saveServo[idx]
                );

				delay(100);
            }
            else if(CLdivRL > -0.8)
            {
               	printf("R: Turn left\n");

				saveServo[idx] = \
                    WHEEL_SET+((CLdivRL - 1) * 5);

				printf(
                    "servo = %f\n",
                    saveServo[idx]
                );

				delay(100);
            }
			else 
            {
				printf("Straight\n");
		    	saveServo[idx] = WHEEL_SET; 
				delay(100);
			}
		}
		else 
        {
			printf("Break\n");
	    	saveServo[idx] = WHEEL_SET; 
			delay(100);
	    }
    }
}
