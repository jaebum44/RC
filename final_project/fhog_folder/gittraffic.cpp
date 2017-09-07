#include <dlib/image_processing.h>
#include <dlib/image_transforms.h>
#include <dlib/gui_widgets.h>
#include <dlib/svm_threaded.h>
#include <dlib/gui_widgets.h>
#include <dlib/data_io.h>
#include <dlib/opencv.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>  
#include <opencv2/imgcodecs.hpp>  

#include <iostream>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <ctime>
#include <cstdio>
#include <cstdlib>

#include <fcntl.h>
#include <sys/socket.h> 
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/shm.h>

#define IMG_KEY		1234
#define SIG_KEY		2345	
#define MEM_SIZE	1024

using namespace std;
using namespace cv;

namespace HSV_CONST
{
	enum
	{
		RED_HL =170, RED_SL =50, RED_VL =120,\
		RED_HH =179, RED_SH =255, RED_VH =255,\
		GREEN_HL =38, GREEN_SL =50, GREEN_VL =120,\
		GREEN_HH =80, GREEN_SH =255, GREEN_VH =255
	};
}

void*img_handler(void*);
int tlight_msg_handler(Mat &, char*);
int tsign_msg_handler(Mat &, int, char*);
void create_msg_box(std::vector<dlib::rectangle> &, dlib::rectangle &);
int hsv_handler(Mat &);
int red_detect(Mat &);
int green_detect(Mat &);
float dist_detect_tl(dlib::rectangle &);
float dist_detect_ts(dlib::rectangle &r);

pid_t pid;
void* shm_addr_img;
void* shm_addr_sig;

int main(int argc, char** argv)
{

	int connSock;
	struct sockaddr_in server_addr;
	char*serverAddr;
	int serverPort; 
	int thr_id, status;
	pthread_t pid;
	int len;

	int	shm_id_img;
	int	shm_id_sig;

	if(argc < 3)
	{
		perror("Usage: IP_address Port_num");
		return -1;
	}

	serverAddr = argv[1];
	serverPort = atoi(argv[2]);

	pid = fork();

	if( pid == 0 )
	{
		if((connSock=socket(PF_INET, SOCK_STREAM, 0)) < 0) 
		{
			perror("Traffic client can't open socket");
			return -1;
		}
	
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = inet_addr(serverAddr);
		server_addr.sin_port = htons(serverPort);
	
		if(connect(connSock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) 
		{
			perror("Traffic client can't connect");
			return -1;
		}
	
		printf("Traffic Client connect to Traffic server \n");
	}

	if(-1 == ( shm_id_img = shmget( (key_t)IMG_KEY, MEM_SIZE, IPC_CREAT)))
	{
		printf( "공유 메모리 생성 실패\n");
		return -1;
	}

	if( ( void *)-1 == ( shm_addr_img = shmat( shm_id_img, ( void *)0, 0)))
	{
		printf( "공유 메모리 첨부 실패\n");
		return -1;
	}

	if(-1 == ( shm_id_sig = shmget( (key_t)SIG_KEY, MEM_SIZE, IPC_CREAT)))
	{
		printf( "공유 메모리 생성 실패\n");
		return -1;
	}

	if( ( void *)-1 == ( shm_addr_sig = shmat( shm_id_sig, ( void *)0, 0)))
	{
		printf( "공유 메모리 첨부 실패\n");
		return -1;
	}

	thr_id=pthread_create(&pid, NULL,img_handler,(void*)&connSock);

	pthread_join(pid, (void**)&status);

	close(connSock);

	return 0;
}

void*img_handler(void*arg)
{
	int connSock=*(int*)arg;
	int detect_color = 0;
	int red_sign_on = 0, child_sign_on = 0, buf = 0;
	
	typedef dlib::scan_fhog_pyramid<dlib::pyramid_down<6> > image_scanner_type; 
	
	image_scanner_type scanner;

	dlib::rectangle tlight_r, tsign_r;
	dlib::object_detector<image_scanner_type> detector_tlight;
	dlib::object_detector<image_scanner_type> detector_tsign;
	dlib::matrix <dlib::bgr_pixel>cimg;
	dlib::image_window win;
		
	while(1)
	{
		clock_t ttime=clock();
		Mat img;

		img = Mat::zeros(120, 400,CV_8UC3);
		   
		int imgSize = img.total() * img.elemSize();
		uchar *iptr = img.data;

		if(!img.isContinuous()) 
		{ 
			img = img.clone();
		}

		clock_t timex=clock();

		if( pid == 0 )
		{
			if(recv(connSock, iptr, imgSize, MSG_WAITALL) < 0) 
			{
				perror("receive from server failed");
			}
			memcpy( shm_addr_img, iptr, sizeof imgSize );
		}
		else if( pid > 0 )
		{
			memcpy( iptr, shm_addr_img, sizeof imgSize );
		}

		Rect roi(img.cols>>1,0,img.cols>>1,img.rows);
		
		img=img(roi);

		
		cout<<"img cols : "<<img.cols<<"img rows : "<<img.rows<<endl;
		cout<<"imgdlib cols : "<<img.cols<<"imgdlib rows : "<<img.rows<<endl;

		cout<<"  received time : "<<clock()-timex<<endl;

		clock_t timey=clock();

		if( pid > 0 )
		{
			char tl_msg[50];

			vector<dlib::rectangle> dets_tlight;
			dlib::assign_image(cimg,dlib::cv_image<dlib::rgb_pixel>(img));
			dlib::pyramid_up(cimg);	

			scanner.set_detection_window_size(80, 80); 
			detect_color = tlight_msg_handler(img, tl_msg);

			if( detect_color )
			{
				dlib::deserialize("tlight2_detector.svm") >> detector_tlight;
	
				dets_tlight = detector_tlight(cimg);
		
				if(dets_tlight.size())
				{
					printf("red sign detected\n");
					//cout<<"dets left : "<<dets_tlight[0].left()<<"dets right : "<<dets_tlight[0].right()<<"dets top : "<<dets_tlight[0].top()<<"dets bottom() : "<<dets_tlight[0].bottom()<<endl;
					red_sign_on  = 2;
				}
				cout<<"dlib time : "<<clock()-timey<<endl;
		
				create_msg_box(dets_tlight, tlight_r);
	
				detect_color = 0;
			}

			memcpy( shm_addr_sig, &red_sign_on, sizeof red_sign_on );

			printf("red_sign_on = %d\n", red_sign_on );

			red_sign_on = 0;

			if(dets_tlight.size())
			{
				tlight_r.bottom()=tlight_r.top()-1;
			}		
		}
		else if( pid == 0 )
		{
			char ts_msg[50];

			vector<dlib::rectangle> dets_tsign;
			dlib::assign_image(cimg,dlib::cv_image<dlib::rgb_pixel>(img));
			dlib::pyramid_up(cimg);	

			scanner.set_detection_window_size(80, 80); 

			dlib::deserialize("tsign_detector.svm") >> detector_tsign;
			dets_tsign = detector_tsign(cimg);

			if(dets_tsign.size())
			{
				printf("child sign detected\n");
				tsign_msg_handler(img, ts_msg);
			}

			create_msg_box(dets_tsign, tsign_r);

			memcpy( &red_sign_on, shm_addr_sig, sizeof red_sign_on );

			buf= red_sign_on | child_sign_on; // 0 fast, 1 slow, 2 stop, 3 slow and stop

			if(send(connSock, &buf, sizeof(buf), 0) < 0 ) 
			{
				perror("send to traffic server failed");
			}
		}

		clock_t timez=clock();
		
		cout<<"window time : "<<clock()-timez<<endl;
		cout<<"one while time : "<<clock()-ttime<<endl;

		cvtColor(img, img, CV_BGR2RGB);
		pyrUp( img, img, Size( img.cols*2, img.rows*2 ) );
		imshow("window", img);
		waitKey(10);
	}

}


void create_msg_box(std::vector<dlib::rectangle> &dets, dlib::rectangle &r)
{
	if(dets.size())
	{
		if(dets[0].left() > 0)
			r.left()=dets[0].left();
		else
			r.left()=0;
		r.bottom()=dets[0].bottom();
		r.right()=dets[0].right();
		if(dets[0].top() > 0)
			r.top()=dets[0].top();
		else
			r.top()=0;
	}
}


int tlight_msg_handler(Mat &img, char*distance_msg)
{
	int i= 0;
	int red_positive;
		
	red_positive = hsv_handler(img);

	if(red_positive%2!=0)
	{
		printf("\nred\n\n");
		cout<<"Stop"<<' '<<i<<endl;
		i++;

		return 2;
	}
	else
	{
		printf("\ngreen\n\n");
		if(red_positive)
		return 2;
	}

	return 0;
}

int tsign_msg_handler(Mat &img, char*distance_msg)
{
	child_sign_on = 1;
	system("mpg123 child_sign_voice.mp3");

	return 0;
}


float dist_detect_tl(dlib::rectangle &r)
{
	//float v = (r.bottom() - r.top())/2+r.top();
	float v = (r.right() - r.left())<<2;
	float distance = 14/tan ( (-3.1) + atan( (v- 119.8) / 332.3 ) );
	cout<< "distance = "<<distance<<"cm"<<endl;
	
	return distance;
}

float dist_detect_ts(dlib::rectangle &r)
{
	//float v = (r.bottom() - r.top())/2+r.top();
	float v = ((r.right() - r.left())<<2)/3;
	float distance = 14/tan ( (-3.1) + atan( (v- 119.8) / 332.3 ) );
	cout<< "distance = "<<distance<<"cm"<<endl;
	
	return distance;
}

int hsv_handler(Mat &img)
{

	Mat img_for_detect[2];
	int red_positive, green_positive;

	green_positive=green_detect(img);
	red_positive=red_detect(img);

	if(green_positive >1)
		return 2;
	else if(red_positive >1)
		return 1;
	else
		return 0;
}

int red_detect(Mat &img_for_detect)
{
	Mat img_binary, img_hsv;

	cvtColor(img_for_detect, img_hsv, COLOR_RGB2HSV); 
	inRange(img_hsv, Scalar(HSV_CONST::RED_HL,HSV_CONST::RED_SL,HSV_CONST::RED_VL),\
		Scalar(HSV_CONST::RED_HH,HSV_CONST::RED_SH,HSV_CONST::RED_VH), img_binary); 
	erode(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
	dilate(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) ); 
	dilate(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) ); 
	erode(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );

	Mat img_labels, stats, centroids;  
	int numOfLables = connectedComponentsWithStats(img_binary, img_labels, stats, centroids, 8, CV_32S);  

	printf("detected reds : %d\n",numOfLables-1);

	return numOfLables;
}

int green_detect(Mat &img_for_detect)
{
	Mat img_binary, img_hsv;

	cvtColor(img_for_detect, img_hsv, COLOR_RGB2HSV); 
	inRange(img_hsv, Scalar(HSV_CONST::GREEN_HL,HSV_CONST::GREEN_SL,HSV_CONST::GREEN_VL),\
		Scalar(HSV_CONST::GREEN_HH,HSV_CONST::GREEN_SH,HSV_CONST::GREEN_VH), img_binary); 
	erode(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
	dilate(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) ); 
	dilate(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) ); 
	erode(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );

	Mat img_labels, stats, centroids;  
	int numOfLables = connectedComponentsWithStats(img_binary, img_labels, stats, centroids, 4, CV_32S);  

	printf("detected greens : %d\n",numOfLables-1);

	return numOfLables;
}

