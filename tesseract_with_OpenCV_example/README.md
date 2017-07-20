## tesseract 설치(leptonica는 1.74 버전 이상으로 설치할 것)

<https://www.linux.com/blog/using-tesseract-ubuntu>

## tesseract 예제 소스

<https://github.com/tesseract-ocr/tesseract/wiki/APIExample>

## tesseract inlcude 오류 해결

<https://groups.google.com/d/msg/tesseract-ocr/r6bL_KLlcyE/B3eMnSvmEwAJ>

## tesseract 컴파일 예시

```
g++ test.cpp -o test -std=c++11 -llept -ltesseract
```

-llept 없이도 컴파일 됨. -llept 역할 파악할 것.

## Mat to Pix, OpenCV와 tesseract 함께 사용

<https://stackoverflow.com/questions/27000797/converting-mat-to-pix-to-setimage>

## tesseract + OpenCV 컴파일 예시

```
g++ test.cpp -o test -std=c++11 -llept -ltesseract `pkg-config --cflags --libs opencv`
```

## dlib 컴파일 옵션

<http://dlib.net/compile.html>

## tesseract + OpenCV + dlib 컴파일 예시

```
g++ client_test.cpp -o client_test -std=c++11 -O3 -I.. /usr/local/include/dlib/all/source.cpp -lpthread -lX11 -ltesseract `pkg-config --cflags --libs opencv`
```

## dlib 컴파일 옵션

<http://dlib.net/compile.html>
