/* test_mydrv.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <wiringPi.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <string.h>
#include <netinet/in.h>

#include "shmhdr.h"
#include "ioctl_mydrv.h"
#define NETLINK_USER 31
#define MAX_PAYLOAD 1024  /* maximum payload size*/
#define MSG_EXCEPT 020000
#define FILE "/dev/sensor"
#define PORT 9000


struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int	c_socket, s_socket;
char	buffer[20]={0};
int sock_fd;
struct msghdr msg;

pthread_t pid[6];

void *Netlink_Socket_thread(void *data);
void *RaspberryPiThread(void *unused); 

typedef struct shared_data{

		char data[50];
		char data1[50];
}shard_data;




int main()
{
	int fd,n,i,size,len,distance;
	int	thr_id, status;
	char out[10]="0";
	char in[10]="1";
	char chardistance[20];
	
	struct sockaddr_in s_addr, c_addr;
	ioctl_buf *buf_in,*buf_out;	

	fd = open(FILE,O_RDWR);	




	thr_id=pthread_create(&pid[0], NULL, Netlink_Socket_thread, (void*)fd);
	if(thr_id<0){
		printf("xx\n");
		return -1;	
	}
	
	thr_id=pthread_create(&pid[1], NULL, RaspberryPiThread, (void *)fd); // 4번째 인자는 매개변수
	if(thr_id<0){
		printf("xx\n");
		return -1;	
	}
	
	pthread_join(pid[0], (void **) &status);// 쓰레드가 끝날 때까지 기다린다면 while문을 돌리는 동안 main문 thread_join에서 걸리게 된다.
	pthread_join(pid[1], (void **) &status);//2번째가 NULL 이 아니라면, th의 리턴값이 저장된 영역이 전달되게 된다.
	return 0;
}

// 넷링크 쓰레

void *Netlink_Socket_thread(void *data){

	struct sockaddr_in s_addr, c_addr;
		ioctl_buf *buf_in,*buf_out;	
	int fd,n,i,size,len,distance,mq;
	int mtype = 0;
	key_t key = 0;
    int shmid;
    void *data_shm;
    int mode;
	shard_data *shdata;

	fd = (int )data;// (int)  (int *)


	// (void * data )
	
	size = sizeof(ioctl_buf);
	buf_out = (ioctl_buf *)malloc(size);
	strcpy(buf_out->data,"0");


	s_socket = socket(PF_INET, SOCK_STREAM, 0);

	memset(&s_addr, 0, sizeof(s_addr));
	s_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	s_addr.sin_family = AF_INET;
	s_addr.sin_port = htons(PORT);
	printf("bind before\n");
	if(bind(s_socket, (struct sockaddr *) &s_addr, sizeof(s_addr)) == -1) {
		printf("Can not Bind\n");
		return;
	}
	printf("bind after\n");
	if(listen(s_socket, 5) == -1) {
		printf("listen Fail\n");
		return;
	}// request connection  it is possible to wait 5 stack. 
	printf("listen after\n");
	len = sizeof(c_addr);
	c_socket = accept(s_socket, (struct sockaddr *) &c_addr, &len);
//------------------------------------------------------------------------------------mesgqueue


    /* make the key: */
    if ((key = ftok("test_mydrv_shm.c", 'R')) == -1) {
        perror("ftok");
        exit(1);
    }

    /* connect to (and possibly create) the segment: */
    if ((shmid = shmget(key, sizeof(shard_data), 0644 | IPC_CREAT)) == -1) {
        perror("shmget");
        exit(1);
    }//ID 받아오는곳

    /* attach to the segment to get a pointer to it: */
    data_shm = shmat(shmid, (void *)0, 0); //공유파일 저장.
	
    if (data_shm == (void *)(-1)) {
        perror("shmat");
        exit(1);
    	}

	shdata = (shard_data *)data_shm;

	while(1)
	{	


//---------------------------------------------------------------------------Netlink---------------------
		sock_fd=socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
		
		if(sock_fd<0)
			return;
		memset(&src_addr, 0, sizeof(src_addr));
		src_addr.nl_family = AF_NETLINK;
		src_addr.nl_pid = getpid();  // self pid 
		// sockaddr_nl 구조체는 사용자 공간 또는 커널에서의 넷링크 소켓을 나타낸다/
		// interested in group 1<<0 
		bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));
		
		memset(&dest_addr, 0, sizeof(dest_addr));
		dest_addr.nl_family = AF_NETLINK;
		dest_addr.nl_pid = 0;   // For Linux Kernel 
		dest_addr.nl_groups = 0; // unicast 
		
		nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD)); //메모리 할당
		memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));// 초기화	//MLMSG_SPACE =저장 영역의 길이가 len 의 netlink 메시지의 바이트 수를 돌려 준다.
		nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD); //MLMSG_SPACE =저장 영역의 길이가 len 의 netlink 메시지의 바이트 수를 돌려 준다.
		nlh->nlmsg_pid = getpid();		//전송 포트 아이디 메시지가 커널에 전송되면 nlmsg_pid는 0이다.
		nlh->nlmsg_flags = 0;			
		

		strcpy(NLMSG_DATA(nlh), "abc"); //보낼 데이터.
		
		iov.iov_base = (void *)nlh;
		iov.iov_len = nlh->nlmsg_len;
		msg.msg_name = (void *)&dest_addr;
		msg.msg_namelen = sizeof(dest_addr);
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;
		//pinMode(SW, INPUT);
		
		
		
		ioctl(fd, IOCTL_MYDRV_WRITE, buf_out );// 구조체를 보내는 방식
		sendmsg(sock_fd,&msg,MSG_WAITALL);
		
		
		// Read message from kernel 
		recvmsg(sock_fd, &msg, MSG_WAITALL);
		printf(" Received message payload: %s\n", NLMSG_DATA(nlh));
//------------------------------------------------------------------------------------Netlink
		
//------------------------------------------------------------------------------------socket
		strcpy(buffer,NLMSG_DATA(nlh));
		n = strlen(NLMSG_DATA(nlh));
	
		strcpy(shdata->data, buffer);
	

		send(c_socket, buffer, n+1,0);
		printf("buffer=%s\n",shdata->data);
		usleep(200000);
		close(sock_fd);	//소켓을 연결하고 다시 닫아야 한다 while문에서  커널과 앱간의 연결하는 디스크립터 
		// close(fd); 이건 socket 라즈베리간의 통신을 뜻함.
	}
     
} 


//순서는 소켓 -> 넷링크 ->ioctl->넷링크recv->socket send

void *RaspberryPiThread(void *unused){ 

 key_t key;
    int shmid;
   
    int mode;
	char buffer[20]={0};
	shard_data *shdata;
	void *data_shm;


    /* make the key: */
    if ((key = ftok("test_mydrv_shm.c", 'R')) == -1) {
        perror("ftok");
        exit(1);
    }

    /* connect to (and possibly create) the segment: */
    if ((shmid = shmget(key, sizeof(shard_data), 0644 | IPC_CREAT)) == -1) {
        perror("shmget");
        exit(1);
    }

    /* attach to the segment to get a pointer to it: */
    data_shm = shmat(shmid, (void *)0, 0);
    if (data_shm == (void *)(-1)) {
        perror("shmat");
        exit(1);
    }
	shdata = (shard_data *)data_shm;
	while(1)
	{
	strcpy(buffer,shdata->data);
   	 printf("RaspberryPi buffer =%s\n",buffer);
	usleep(500000);
	}
    /* detach from the segment: */
    if (shmdt(data_shm) == -1) {
        perror("shmdt");
        exit(1);
    }

    return 0;
}


/*

struct nlmsghdr {
    __u32 nlmsg_len;    // Length of message including header. 
    __u16 nlmsg_type;   // Type of message content. 
    __u16 nlmsg_flags;  // Additional flags. 
    __u32 nlmsg_seq;    // Sequence number. 
    __u32 nlmsg_pid;    // Sender port ID. 
};

*/

