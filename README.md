Bit Autonomous RC Car Project 
======================

Note: this repository was created for the final project of IoT embedded system developer course and won't receive any major updates. There are methods with better results than HoG for traffic sign detector, such as Deep Learning architectures. Still, you can use this repository as a study reference or for some practical purposes.

This is a traffic sign detector and classifier that uses dlib and its implementation of the Felzenszwalb's version of the Histogram of Oriented Gradients (HoG) detector.

The training examples used in this repository are from Korean road signs, but the classifier should work with any traffic signs, as long as you train it properly. Naver Map images can be used to train the detectors. 25~40 images are sufficient to train a good detector.

| junghwk | yeogue | jaebum44 | desung7 |
| :---: | :---: | :---: | :---: |
| OpenCV | OpenCV | OpenCV | OpenCV |
| dlib | dlib | device driver | dlib |
| dlib | dlib | device driver | dlib |


## Table of content

- [Introduction](#introduction)
    - [Development Environment](#development-environment)
    - [Block Diagram](#block-diagram)
    - [Flow Chart](#flow-chart)
    
- [Computer Vision](#computer-vision)
    - [OpenCV](#openCV)
    - [dlib](#dlib)
    
- [Device Driver](#device-driver)
    - [Upload the page tree file](#upload-the-page-tree-file)
    - [Go to the import view](#go-to-the-import-view)
    - [Import the uploaded page tree file](#import-the-uploaded-page-tree-file)

- [](#license)

- [Links](#links)

## Introduction

### Development Environment

line detection
* Log into the TYPO3 back end
* Click on ''Admin Tools::Extension Manager'' in the left navigation
* Click the icon with the little plus sign left from the Aimeos list entry (looks like a lego brick)
* If a pop-up opens (only TYPO3 4.x) choose ''Make updates'' and "Close window" after the installation is done

**Caution:** Install the **RealURL extension before the Aimeos extension** to get nice looking URLs. Otherwise, RealURL doesn't rewrite the parameters even if you install RealURL afterwards!
<https://github.com/opencv/opencv>

### Block Diagram

![bd](./img/block_diagram.jpg)

### Flow Chart

![fc](./img/flow_chart.jpg)

## OpenCV

line detection
* Log into the TYPO3 back end
* Click on ''Admin Tools::Extension Manager'' in the left navigation
* Click the icon with the little plus sign left from the Aimeos list entry (looks like a lego brick)
* If a pop-up opens (only TYPO3 4.x) choose ''Make updates'' and "Close window" after the installation is done

**Caution:** Install the **RealURL extension before the Aimeos extension** to get nice looking URLs. Otherwise, RealURL doesn't rewrite the parameters even if you install RealURL afterwards!
<https://github.com/opencv/opencv>

## dlib

### install

```
git clone https://github.com/davisking/dlib.git
```

### Build

```
sudo su
cd dlib/example
mkdir build
cd build 
cmake .. -DUSE_AVX_INSTRUCTIONS=1
cmake --build . --config Release
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

### Mark signs on images

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

### Train the fHOG_object_detector

To train a fHOG_object_detector, run `fhog_object_detector ex`. For example, to run the detector on the `/dir/image` folder with .xml files in it. 

```
dlib/example/build/fhog_object_detector_ex /path/to/dir/image
```

The detector will show test images with marks on and be saved to the file `face_detector.svm`.

### How to reuse face_detector.svm in your project 

```
cd dlib/example/
cp fhog_object_detector_ex.cpp fhog_object_detector2_ex.cpp
g++ -std=c++11 -O3 -I.. ../dlib/all/source.cpp -lpthread -lX11 -DDLIB_JPEG_SUPPORT -ljpeg fhog_object_detector2_ex.cpp -o fhog_object_detector2_ex `pkg-config --cflags --libs opencv` 
```
Note :  This format is crucial. `yourownfile_ex.cpp -o yourownfile_ex` 

Note : There's another way to compile using `make` in dir/build. In this case you have to make your own customized CMakeLists.txt 

## motor control


## device driver


## TCP/IP network


## schedule management

project planning of team

write journal

<https://drive.google.com/open?id=181hj2BIQw41JlRvIvNAiyvcg5SGY2thXSMtJz_hX9zQ>

