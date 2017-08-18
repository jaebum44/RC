#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <fcntl.h>
#include <linux/netlink.h>
#include <signal.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>

#include <errno.h>

#include "opencv2/opencv.hpp"

#include "shmhdr.h"
#include "ioctl_mydrv.h"

#include <wiringPi.h>

#define NETLINK_USER 31
#define MAX_PAYLOAD 1024  /* maximum payload size*/
#define MSG_EXCEPT 020000
#define PATH "/dev/sensor"
#define PORT 9000

#define COMM_PACK 3
#define SEND_PACK1 24
#define SEND_PACK2 25

using namespace std;
using namespace cv;

typedef struct shared_data{
	int ultra_sonic_value;
	int traffic_sign_value;
}shard_data;

int netlink_F( void );
int print_F( int argc, char** argv );

void*netlink_thread(void*);
void*netlink_shared_mem_thread(void*);
void*print_shared_mem_thread(void*);
void*display(void*);
void*send_pack(void*);

void calc_vals( void );

struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
struct msghdr msg;

int sock_fd;
pthread_t netlink_pid[2];

int connSock; 
pthread_t print_pid[2];
int traffic_sign;
int pack_idx = 0;

int*dist;

shard_data car_ctl_T;

int packet[ ][ COMM_PACK ] = \
	{  0, 0,	// stop
	   0, 1,	// slow
	   1, 0 };	// fast

void calc_vals( void )
{
	int a;

	wiringPiSetup();

	pinMode( SEND_PACK1, OUTPUT );
	pinMode( SEND_PACK2, OUTPUT );
	
	// sonic == 1;	stop
	// sign == 1;	slow
	// else;	fast

	if( car_ctl_T.ultra_sonic_value )
	{
		pack_idx = 0;
		// send stop
		digitalWrite( SEND_PACK1, packet[ pack_idx ][ 0 ] );
		digitalWrite( SEND_PACK2, packet[ pack_idx ][ 1 ] );
	}
	else if( traffic_sign )
	{
		pack_idx = 1;
		// send slow
		digitalWrite( SEND_PACK1, packet[ pack_idx ][ 0 ] );
		digitalWrite( SEND_PACK2, packet[ pack_idx ][ 1 ] );
	}
	else
	{
		pack_idx = 2;
		// send fast
		digitalWrite( SEND_PACK1, packet[ pack_idx ][ 0 ] );
		digitalWrite( SEND_PACK2, packet[ pack_idx ][ 1 ] );
	}
}

void* send_pack( void* arg )
{
	while( 1 )
	{
		calc_vals();
	}
} 

int main( int argc, char** argv )
{
	int		fork_pid, status;
	pthread_t	main_thrd;

	pthread_create( &main_thrd, NULL, &send_pack, NULL );

	fork_pid = fork();

	if( fork_pid == 0 )
	{
		netlink_F();
		pause();
	}
	else if( fork_pid > 0 )
	{
		print_F( argc, argv );
		pause();
	}
	else
	{
		perror( "fork error\n" );
		return -1;
	}

	pthread_join( main_thrd, ( void** )&status );

	return 0;
}

int netlink_F()
{
	int fd, thr_id, status;

	struct sockaddr_in s_addr, c_addr;

	fd = open(PATH,O_RDWR);	


	if((thr_id=pthread_create(&netlink_pid[0], NULL, netlink_thread, (void*)&fd)) < 0)
	{
		perror("xx\n");
		return -1;	
	}
	if((thr_id=pthread_create(&netlink_pid[1], NULL, netlink_shared_mem_thread, (void*)&fd)) < 0)
	{
		printf("xx\n");
		return -1;	
	}
	
	pthread_join(netlink_pid[0], (void**)&status);// 쓰레드가 끝날 때까지 기다린다면 while문을 돌리는 동안 main문 thread_join에서 걸리게 된다.
	pthread_join(netlink_pid[1], (void**)&status);//2번째가 NULL 이 아니라면, th의 리턴값이 저장된 영역이 전달되게 된다.
	return 0;
}

// 넷링크 쓰레

void*netlink_thread(void*arg)
{
	int i=0;
	ioctl_buf *buf_in,*buf_out;	
	int fd, size, len;
	int distance;

	key_t key = 0;
	int shmid;
	void*data_shm;
	shard_data *shdata;

	fd = *(int*)arg;

	dist=(int*)malloc(sizeof(int));
	size = sizeof(ioctl_buf);
	buf_out = (ioctl_buf*)malloc(size);
	strcpy(buf_out->data,"0");

	while(1)
	{	
		sock_fd=socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
		
		if(sock_fd<0)
			return NULL;
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
		
		memcpy(NLMSG_DATA(nlh),dist,sizeof(int));
		
		iov.iov_base = (void *)nlh;
		iov.iov_len = nlh->nlmsg_len;
		msg.msg_name = (void *)&dest_addr;
		msg.msg_namelen = sizeof(dest_addr);
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;
		
		ioctl(fd, IOCTL_MYDRV_WRITE, buf_out);// 구조체를 보내는 방식
		
		sendmsg(sock_fd,&msg,0);
		// Read message from kernel 

		recvmsg(sock_fd, &msg, 0);
	    dist=((int*)NLMSG_DATA(nlh));
	    printf("Received message payload : %d cm %d\n",*dist,i);
		usleep(200000);
		i++;
		
		if(*dist < 20)
			car_ctl_T.ultra_sonic_value = 1;
		else
			car_ctl_T.ultra_sonic_value = 0;

		close(sock_fd);	//소켓을 연결하고 다시 닫아야 한다 while문에서  커널과 앱간의 연결하는 디스크립터 
	}

	free(dist);
     
} 


//순서는 소켓 -> 넷링크 ->ioctl->넷링크recv->socket send

void*netlink_shared_mem_thread(void*arg)
{ 

	key_t key;
	int shmid;

	int mode;
	int save_distance=0;
	shard_data *shdata;
	void *data_shm;


	/* make the key: */
	if ((key = 1234) == -1) {//ftok("test_mydrv_shm.c", 'R')) == -1) {
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
		printf("RaspberryPi buffer =%d\n",shdata->ultra_sonic_value);
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





//////////////////////////

int print_F(int argc, char **argv) 
{
	int i=0;
	int len;
	int thr_id[2];
	int status;
	int listenSock;

	struct sockaddr_in client_addr, server_addr;

	

	if(argc < 2)
	{
		perror("Usage: Server port_num \n");
		return -1;
	}

	if((listenSock=socket(PF_INET, SOCK_STREAM, 0)) < 0) 
	{
		perror("Server: Can't open socket\n");
		return -1;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET; // protocol address type, if IPv4 == AF_INET
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 서버의 아이디를 그대로 사용함
	server_addr.sin_port = htons(atoi(argv[1])); // port num

	if(bind(listenSock, (struct sockaddr*) &server_addr, sizeof(server_addr))<0)
	{
		perror("Server: Can't bind \n");
		return -1;
	}

	if(listen(listenSock, 1) < 0) // 1 == backlog == which means the maximum number of delayed connection try allowed to wait in stack.  
	{
		perror("Server: Listen failed \n");
		return -1;
	}

	len = sizeof(client_addr);

	cout<<"Traffic Server Start!\nIP = "<<htonl(INADDR_ANY)<<" "<<"Port Num = "<<atoi(argv[1])<<endl;
	while(1)
	{
		if((connSock = accept(listenSock,(struct sockaddr*)&client_addr,(socklen_t*)&len))<0)
		{
			perror("Server: Failed in accepting \n");
			return -1;
		}
		
		printf("Server: Accept new request \n");

		thr_id[0] = pthread_create(&print_pid[0], NULL, display, (void*)&connSock);		
		thr_id[1] = pthread_create(&print_pid[1], NULL, print_shared_mem_thread, NULL);
		
	}


	pthread_join(print_pid[0], (void**)&status);
	pthread_join(print_pid[1], (void**)&status);

	//close(listenSock);
	//close(connSock);
}

void*display(void*arg)
{
	int connSock=*(int*)arg;
	int bytes;

	Mat img;
	Mat imgGray;

	img = Mat::zeros(240 ,320, CV_8UC3);  
	int imgSize = img.total() * img.elemSize();
	while(1) 
	{
		VideoCapture cap(0);
	 
		cap.set(CAP_PROP_FRAME_WIDTH,320);
		cap.set(CAP_PROP_FRAME_HEIGHT,240);
		cap>>img;
		
		
		cvtColor(img, imgGray, CV_BGR2RGB);

		if(!img.isContinuous()) 
		{ 
			img = img.clone();
			imgGray = img.clone();
		}
		
		
		if(bytes=send(connSock, imgGray.data, imgSize, 0) < 0)
		{
			cerr<<"bytes send failed = "<<bytes<<endl;
		}
			
		if(recv(connSock, &traffic_sign, sizeof(int), MSG_WAITALL) < 0) 
		{
			perror("traffic_sign receive fail");
		}

		if(!traffic_sign)
			cout<<"detected!! traffic_sign == "<<traffic_sign<<endl;
		else
			cout<<"no signs.... traffic_sign == "<<traffic_sign<<endl;
		
	}



}


void*print_shared_mem_thread(void*arg)
{
	key_t key;
	int shmid;
	int mode;
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
		shdata->traffic_sign_value = traffic_sign;
	
		if(!traffic_sign)
			cout<<"detected!! traffic_sign == "<<traffic_sign<<endl;
		else
			cout<<"no signs.... traffic_sign == "<<traffic_sign<<endl;
		
	}



}
