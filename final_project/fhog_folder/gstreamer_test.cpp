#include <dlib/image_transforms.h>
#include <dlib/opencv.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>
#include <dlib/svm_threaded.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_processing.h>
#include <dlib/data_io.h>

#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/opencv.hpp"

#include <iostream>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <ctime>
#include <cstdio>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/socket.h> 
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>


using namespace std;
using namespace dlib;

cv::Mat img;


void*fhog_detect(void*arg);

int main()
{ 
	int thr_id, status;
	pthread_t pid;
	int len;
        cv::Mat* img_main = &img;
	*img_main = cv::Mat::zeros(480, 640, CV_8UC3); 
	cv::VideoCapture vcap; 
 
	string videoStreamAddress = "rtsp://192.168.1.33:8554/test";	
	
	vcap.open(videoStreamAddress);
	
	thr_id=pthread_create(&pid, NULL,fhog_detect,NULL);


	for(;;) { 
		vcap.read(*img_main); 

      		cv::imshow("Output Window", *img_main); 
  	        cv::waitKey(1);
//			break; 
        } 	
	
//	thr_id=pthread_create(&pid, NULL,fhog_detect,NULL);

	pthread_join(pid, (void**)&status);
	

	return 0;
}

void*fhog_detect(void*arg)
{
//	cv::Mat img;
//	img = cv::Mat::zeros(240, 320, CV_8UC3);
	cv::Mat* img_main = &img;   
	typedef scan_fhog_pyramid<pyramid_down<6> > image_scanner_type; 
	image_scanner_type scanner;
	scanner.set_detection_window_size(40, 40); 
	object_detector<image_scanner_type> detector;
	deserialize("tsign_detector.svm") >> detector;
	matrix <bgr_pixel>cimg;
	image_window win;

	int i=0;
	int sign_on = 0;

//	cv::VideoCapture vcap; 
 
//	string videoStreamAddress = "rtsp://192.168.1.33:8554/test";	
	
//	vcap.open(videoStreamAddress);

//	for(;;) { 
  //        vcap.read(img); 
//
 //      cv::imshow("Output Window", img); 
  //        if(cv::waitKey(1) >= 0)
//	  break; 
//        } 

        while(1)
	{ 
		int imgSize = (img_main->total()) * (img_main->elemSize());
		uchar *iptr = img_main->data;
		int buf;
		
//		vcap.read(img); 

//	        cv::imshow("Output Window", img); 
//	        cv::waitKey(1);
//		if(!img.isContinuous()) 
//		{ 
//			img = img.clone();
//		}

		assign_image(cimg,cv_image<rgb_pixel>(*img_main));
		//cimg.set_size(240,320);
//		pyramid_up(cimg);
		std::vector<rectangle> dets = detector(cimg);

		rectangle r;
		char distance_msg[20];


		if(dets.size())
		{
			
			r.left()=dets[0].left();
			r.bottom()=dets[0].bottom();
			r.right()=dets[0].right();
			r.top()=dets[0].top();
			//int v = (dets[0].bottom() - dets[0].top())/2 + dets[0].top();
			float v = dets[0].bottom()/2;
			float distance = 13.3/tan ( 10 + atan( (v- 119.8) / 332.3 ) );
			cout<< "distance = "<<distance<<"cm"<<endl;
			sprintf(distance_msg,"Distance : %.2f\n",distance);

			if( distance > 20 )
				buf = 0;
			else
			{
				sign_on = 1;
				buf = 1;
			}

		}
		else
		{
			cout<<dets.size()<<endl;

			if( !sign_on )
				buf=0;
			else
				buf=1;

		}


		win.clear_overlay();
		win.set_image(cimg);
		win.add_overlay(dets, rgb_pixel(255,0,0));

		if(dets.size())			
			win.add_overlay(r, rgb_pixel(255,255,0),distance_msg);
	}

	return 0;
}	
