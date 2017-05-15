#include"./utili1.h"

int main(int argc,char *argv[])
{
	const char *ip = argv[1];
	int port = atoi(argv[2]);

	struct sockaddr_in addrSer;
	bzero(&addrSer,sizeof(addrSer));
	addrSer.sin_family = AF_INET;
	addrSer.sin_port = htons(port);
	inet_pton(AF_INET,ip ,&addrSer.sin_addr);

	int sock = socket(AF_INET,SOCK_STREAM,0);
	assert(sock>=0);
	
	int ret = connect(sock,(struct sockaddr*)&addrSer,sizeof(addrSer));
	assert(ret>=0);

	char buf[50];


	while(1)
	{
		scanf("%s",buf);
		ret = send(sock,buf,strlen(buf),0);
		if(ret<0)
		{
			printf("errno = %d\n",errno);
			break;
		}
/*		ret = recv(sock,buf,38,MSG_DONTWAIT );
		if(ret!=0)
		{
			printf("%s",buf);
			break;
		}
*/	
	}
/*
	while(1)
	{

		ret = recv(sock,buf,50,0);
		if(ret<0)
		break;
		else if(ret == 0)
		{
			printf("sever close connextion\n");
			break;
		}
		else 
		printf("get %d bytes :  %s\n",(int)strlen(buf),buf);
	}
*/
	close(sock);
}
