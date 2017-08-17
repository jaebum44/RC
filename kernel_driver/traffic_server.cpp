#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>

#include "opencv2/opencv.hpp"

using namespace std;
using namespace cv;

int connSock; 
pthread_t pid[2];
int traffic_sign;

typedef struct shared_data{
	int ultra_sonic_value;
	int traffic_sign_value;
}shard_data;

void*display(void*);
void*shared_mem_thread(void*);

int main(int argc, char **argv) 
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

		thr_id[0] = pthread_create(&pid[0], NULL, display, (void*)&connSock);		
		thr_id[1] = pthread_create(&pid[1], NULL, shared_mem_thread, NULL);
		
	}


	pthread_join(pid[0], (void**)&status);
	pthread_join(pid[1], (void**)&status);

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


void*shared_mem_thread(void*arg)
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
		shdata->traffic_sign_value =traffic_sign;
	
		if(!traffic_sign)
			cout<<"detected!! traffic_sign == "<<traffic_sign<<endl;
		else
			cout<<"no signs.... traffic_sign == "<<traffic_sign<<endl;
		
	}



}
