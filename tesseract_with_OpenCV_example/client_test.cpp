#include <dlib/image_transforms.h>
#include <dlib/opencv.h>
#include <opencv2/highgui/highgui.hpp>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>

#include <dlib/svm_threaded.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_processing.h>
#include <dlib/data_io.h>



#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>

#include <iostream>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <fcntl.h>

#include <sys/socket.h> 
#include <arpa/inet.h>
#include <unistd.h>

#include <time.h>

#include <string>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

using namespace std;
using namespace dlib;

int main(int argc, char** argv)
{
    char* outText;

    //--------------------------------------------------------
    //networking stuff: socket , connect
    //--------------------------------------------------------
    int         sokt;
    char*       serverIP;
    int         serverPort;

    if (argc < 3) {
           std::cerr << "Usage: cv_video_cli <serverIP> <serverPort> " << std::endl;
    }

    serverIP   = argv[1];
    serverPort = atoi(argv[2]);

    struct  sockaddr_in serverAddr;
    socklen_t           addrLen = sizeof(struct sockaddr_in);

    if ((sokt = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "socket() failed" << std::endl;
    }

    serverAddr.sin_family = PF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(serverIP);
    serverAddr.sin_port = htons(serverPort);

    if (connect(sokt, (sockaddr*)&serverAddr, addrLen) < 0) {
        std::cerr << "connect() failed!" << std::endl;
    }



    //----------------------------------------------------------
    //OpenCV Code
    //----------------------------------------------------------

    typedef scan_fhog_pyramid<pyramid_down<6> > image_scanner_type; 
    image_scanner_type scanner;
    scanner.set_detection_window_size(40, 40); 
    object_detector<image_scanner_type> detector;
    deserialize("face_detector.svm") >> detector;
    matrix <bgr_pixel>cimg;
    int i=0;
    image_window win;

    while(1)
    {
	    cv::Mat img;
	    img = cv::Mat::zeros(480 , 640, CV_8UC3);    
	    int imgSize = img.total() * img.elemSize();
	    uchar *iptr = img.data;
	    int bytes = 0;
	    int key;

	    //make img continuos
	    if ( ! img.isContinuous() ) { 
		  img = img.clone();
	    }
		
	    
	    while (key != 'q') {

		if ((bytes = recv(sokt, iptr, imgSize , MSG_WAITALL)) == -1) {
		    std::cerr << "recv failed, received bytes = " << bytes << std::endl;
		}
		  
		if (key = cv::waitKey(10) >= 0) break;
	    }   

	    assign_image(cimg,cv_image<rgb_pixel>(img));
	    //assign_image(cimg2,cv_image<rgb_pixel>(cimg));
	    cimg.set_size(480,640);
            pyramid_up(cimg);
	    std::vector<rectangle> dets = detector(cimg);
	    
	    if(dets.size() >0)
	    {
		cout<<"jackpot "<<i<<endl;
		i++;
	    }
	    else
		cout<<dets.size()<<endl;
	    
	    win.clear_overlay();
            win.set_image(cimg);
            win.add_overlay(dets, rgb_pixel(255,0,0));

	    //cv::Mat simg=toMat(cimg);
            	  
	    //cv::imshow("result",simg);
	    //cin.get();
	    //cv::waitKey(0);

	    
    }
    close(sokt);

    return 0;
}
