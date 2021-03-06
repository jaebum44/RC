#include <string>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

#include <opencv2/opencv.hpp>

using namespace std;

int main(int argc, char** argv)
{
    char *outText;
    cv::Mat _mat = cv::imread(argv[2]);
    cv::cvtColor(_mat, _mat, CV_BGR2RGBA); 

    tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
    // Initialize tesseract-ocr(second argument is option -l), without specifying tessdata path.
    if (api->Init(NULL, argv[1])) {
        fprintf(stderr, "Could not initialize tesseract.\n");
        exit(1);
    }

    // Open input image with leptonica library
    // Pix *image = pixRead(argv[2]);
    api->SetImage(_mat.data, _mat.cols, _mat.rows, 4, 4*_mat.cols); // (image);
    // Get OCR result
    outText = api->GetUTF8Text();
    printf("OCR output:\n%s", outText);

    // Destroy used object and release memory
    api->End();
    delete [] outText;
    // pixDestroy(&image);

    return 0;
}
