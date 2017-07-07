#include <dlib/svm_threaded.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_processing.h>
#include <dlib/data_io.h>

#include <iostream>
#include <cstdio>
#include <cstring>
#include <fstream>

#include <fcntl.h>

#include "opencv2/imgproc/imgproc.hpp"


using namespace std;
using namespace dlib;


int main(int argc, char** argv)
{  

	array2d<unsigned char> img;
        load_image(img, argv[1]);
    try
    {
	pyramid_up(img);
        typedef scan_fhog_pyramid<pyramid_down<6> > image_scanner_type; 
        image_scanner_type scanner;


        scanner.set_detection_window_size(80, 80); 

        object_detector<image_scanner_type> detector;
	deserialize("face_detector.svm") >> detector;
	
	image_window hogwin(draw_fhog(detector), "Learned fHOG detector");

        image_window win; 
        std::vector<rectangle> dets = detector(img);
        win.clear_overlay();
        win.set_image(img);
        win.add_overlay(dets, rgb_pixel(255,0,0));
        cin.get();

    }
    catch (exception& e)
    {
        cout << "\nexception thrown!" << endl;
        cout << e.what() << endl;
    }
}

// ----------------------------------------------------------------------------------------

