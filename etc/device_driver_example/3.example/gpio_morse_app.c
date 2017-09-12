#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/kdev_t.h>
 
/* mknod로 생성한 장치 파일의 위치 */
#define _MORSE_PATH_ "/dev/morse"
 
int main(int argc, char *argv[]){
    int fd = 0;
    /* 이 함수를 사용하면 mknod 명령어를 사용하지 않아도 됨
    원본 소스 코드에서는 주석처리 되어 있으므로 mknod 명령어를 사용해야 함 */
    //mknod(_MORSE_PATH_, S_IRWXU | S_IRWXG | S_IFCHR, MKDEV(230, 0));
    /* 장치 파일을 여는 순간 커널 메시지 출력 */
    if((fd = open( _MORSE_PATH_, O_RDWR | O_NONBLOCK)) < 0){
        perror("open()");
        exit(1);
    }
    /* open() 후 메시지를 콘솔에 출력하고 2초간 잠듦 */
    printf("open sungkong!\n"); sleep(2);
    /* 파일을 닫는 순간 커널 메시지 출력 */
    close(fd);
    return 0;
}
