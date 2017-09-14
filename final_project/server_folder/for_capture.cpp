#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace cv;
using namespace std;

int main()
{
	Mat img;
	VideoCapture cap(0);

	char savefile[200];

	for(int i=0;;)
	{
		cap >> img;
		imshow("image",img);
		sprintf(savefile,"image%d.jpg",i++);
		imwrite(savefile,img);
		waitKey(100);

	}

	return 0;
}
