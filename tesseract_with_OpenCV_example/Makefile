SOURCE_NAME = client_test
TESSERACT = -ltesseract -llept
OPENCV = pkg-config --cflags --libs opencv
DLIB = -std=c++11 -O3 -I.. /usr/local/include/dlib/all/source.cpp -lpthread -lX11

client_test : $(SOURCE_NAME).cpp
	g++ $(SOURCE_NAME).cpp -o $(SOURCE_NAME) $(DLIB) $(TESSERACT) `$(OPENCV)` 

clean :
	rm $(SOURCE_NAME)
