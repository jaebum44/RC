#include <iostream>  
#include <opencv2/core/mat.hpp>  
#include <opencv2/imgcodecs.hpp>  
#include <opencv2/imgproc.hpp>  
#include <opencv2/highgui.hpp>
 
 
using namespace cv;
using namespace std;
 
 
 
int main(  )
{
    VideoCapture cap(0);
    
    //웹캡에서 캡처되는 이미지 크기를 320x240으로 지정  
    cap.set(CAP_PROP_FRAME_WIDTH,320);  
    cap.set(CAP_PROP_FRAME_HEIGHT,240);
    
    if ( !cap.isOpened() )
    {
        cout << "웹캠을 열 수 없습니다." << endl;
        return -1;
    }
 
    
//    namedWindow("찾을 색범위 설정", CV_WINDOW_AUTOSIZE); 
 
 
    //트랙바에서 사용되는 변수 초기화 
     int LowH = 170;
     int HighH = 179;
 
      int LowS = 50; 
     int HighS = 255;
 
      int LowV = 142;
     int HighV = 255;
 
    
    //트랙바 생성 
//     cvCreateTrackbar("LowH", "찾을 색범위 설정", &LowH, 179); //Hue (0 - 179)
//     cvCreateTrackbar("HighH", "찾을 색범위 설정", &HighH, 179);

//     cvCreateTrackbar("LowS", "찾을 색범위 설정", &LowS, 255); //Saturation (0 - 255)
//     cvCreateTrackbar("HighS", "찾을 색범위 설정", &HighS, 255);
 
//     cvCreateTrackbar("LowV", "찾을 색범위 설정", &LowV, 255); //Value (0 - 255)
//     cvCreateTrackbar("HighV", "찾을 색범위 설정", &HighV, 255);
 
  
      while (true)
     {
    
        Mat img_input, img_hsv, img_binary;
        
 
        //카메라로부터 이미지를 가져옴 
        bool ret = cap.read(img_input); 
 
        if (!ret) 
        {
            cout << "카메라로부터 이미지를 가져올 수 없습니다." << endl;
            break;
        }
 
     
        //HSV로 변환
        cvtColor(img_input, img_hsv, COLOR_BGR2HSV); 
        
        
        //지정한 HSV 범위를 이용하여 영상을 이진화
        inRange(img_hsv, Scalar(LowH, LowS, LowV), Scalar(HighH, HighS, HighV), img_binary); 
            
 
        //morphological opening 작은 점들을 제거 
        erode(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
        dilate( img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) ); 
 
 
        //morphological closing 영역의 구멍 메우기 
        dilate( img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) ); 
        erode(img_binary, img_binary, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
 
 
        //라벨링 
        Mat img_labels,stats, centroids;  
        int numOfLables = connectedComponentsWithStats(img_binary, img_labels,   
                                                   stats, centroids, 8,CV_32S);  

		printf("label : %d\n",numOfLables); // black == 1, red >= 2 
 
 /*
        //영역박스 그리기
        int max = -1, idx=0; 
        for (int j = 1; j < numOfLables; j++) {
            int area = stats.at<int>(j, CC_STAT_AREA);
            if ( max < area ) 
            {
                max = area;
                idx = j;
            }
        }
            
            
              
        int left = stats.at<int>(idx, CC_STAT_LEFT);  // idx=0, idx=1
        int top  = stats.at<int>(idx, CC_STAT_TOP);  
        int width = stats.at<int>(idx, CC_STAT_WIDTH);  
        int height  = stats.at<int>(idx, CC_STAT_HEIGHT);  
 
 
        rectangle( img_input, Point(left,top), Point(left+width,top+height),  
                    Scalar(0,0,255),1 );  
            
        
        imshow("이진화 영상", img_binary); 
        imshow("원본 영상", img_input); 
 
 
        //ESC키 누르면 프로그램 종료
        if (waitKey(1) == 27) 
            break; 
*/ 
   }
 
    
       return 0;
}