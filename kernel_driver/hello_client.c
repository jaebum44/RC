#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>

#define PORT 9000
//#define IPADDR "169.168.1.98"//서버의 아이피를 따온다.
//#define IPADDR "169.168.1.66"
#define IPADDR "127.0.0.1" //반복루프
int main( )
{
	int	c_socket;
	struct sockaddr_in c_addr;
	int	len;
	int	n;
	
	char	rcvBuffer[BUFSIZ];
	
	c_socket = socket(PF_INET, SOCK_STREAM, 0);
	
	memset(&c_addr, 0, sizeof(c_addr));
	c_addr.sin_addr.s_addr = inet_addr(IPADDR);
	c_addr.sin_family = AF_INET;
	c_addr.sin_port = htons(PORT);
	
	if(connect(c_socket, (struct sockaddr *) &c_addr, sizeof(c_addr))==-1) {
		printf("Can not connect\n");
		close(c_socket);
		return -1;
	}
	while(1){
	if((n = recv(c_socket, rcvBuffer, sizeof(rcvBuffer),0)) < 0) {
		return (-1);
	}
	
	//rcvBuffer[n] = '\0';
	printf("received Data : %s\n", rcvBuffer);

	
	}
	close(c_socket);
	
	return 0;
}
