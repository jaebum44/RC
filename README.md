
| 작성자 |
| :---: |
| junghwk |

# traffic signs detection using fhog_object_detector

## Build

```
sudo su
cd dlib/example
mkdir build
cd build 
cmake ..
cmake --build .
```

if an example requires GUI, check this macro below to DLIB_NO_GUI_SUPPORT=OFF and confirm in CMakeLists.txt
```
macro(add_gui_example name)
   if (DLIB_NO_GUI_SUPPORT=OFF)
      message("No GUI support, so we won't build the ${name} example.")
   else()
      add_example(${name})
   endif()
endmacro()
```

## Mark signs on images

1. Compile `imglab`:

```
cd dlib/tools/imglab
mkdir build
cd build
cmake ..
cmake --build .
```

2. Create XML from sample images:

```
dlib/tools/imglab/build/imglab -c training.xml /path/to/dir/train
dlib/tools/imglab/build/imglab training.xml
dlib/tools/imglab/build/imglab -c testing.xml /path/to/dir/test
dlib/tools/imglab/build/imglab testing.xml
```

3. Use `shift+click` to draw a box around signs.

## Train the fHOG_object_detector

To train a fHOG_object_detector, run `fhog_object_detector ex`. For example, to run the detector on the `/dir/image` folder with .xml files in it. 

```
dlib/example/build/fhog_object_detector_ex /path/to/dir/image
```

The detector will show test images with marks on and be saved to the file `face_detector.svm`.

## How to reuse face_detector.svm in your project 

```
cd dlib/example/
cp fhog_object_detector_ex.cpp fhog_object_detector2_ex.cpp
g++ -std=c++11 -O3 -I.. ../dlib/all/source.cpp -lpthread -lX11 -DDLIB_JPEG_SUPPORT -ljpeg fhog_object_detector2_ex.cpp -o fhog_object_detector2_ex `pkg-config --cflags --libs opencv` 
```
Note :  This format is crucial. `yourownfile_ex.cpp -o yourownfile_ex` 

Note : There's another way to compile using `make` in dir/build. In this case you have to make your own customized CMakeLists.txt 
