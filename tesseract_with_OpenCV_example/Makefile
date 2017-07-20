client_test : client_test.cpp
	g++ client_test.cpp -o client_test -std=c++11 -O3 -I.. /usr/local/include/dlib/all/source.cpp -lpthread -lX11 -ltesseract `pkg-config --cflags --libs opencv`

clean :
	rm client_test
