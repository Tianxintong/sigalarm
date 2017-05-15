#include"./utili.h"
#include"./min_heap.h"

#define FD_LIMIT 65535
#define MAX_EVENT_NUMBER 1024
#define TIMESLOT 10
using namespace std;
static int pipefd[2];
static time_heap th(10);
static int epollfd = 0;

int setnonblocking(int fd)
{
	int old_option = fcntl(fd,F_GETFL);
	int new_option = old_option|O_NONBLOCK;
	fcntl(fd,F_SETFL,new_option);
	return old_option;
}
void addfd(int epollfd,int fd)
{
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;
	epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
	setnonblocking(fd);
}
void sig_handler(int sig)
{
	int save_errno = errno;
	int msg = sig;
	send(pipefd[1],(char*)&msg,1,0);
	errno = save_errno;
}
void time_handler()
{
	th.tick();
	if(!th.empty())
	alarm((th.top()->expire-time(NULL)));//将定时器中最小超时时间（即堆顶计时器的超时时间）作为下一次心搏时间，这样可以减少tick函数被调用的次数，保证每次调用必然都有超时事件发生
	else
	alarm(TIMESLOT);
}
void addsig(int sig)
{
	struct sigaction sa;
	memset(&sa,'\0',sizeof(sa));
	sa.sa_handler = sig_handler;
	sa.sa_flags |=SA_RESTART;
	sigfillset(&sa.sa_mask);
	assert(sigaction(sig,&sa,NULL)!=-1);
}
void cb_func(client_data* user_data)
{
	epoll_ctl(epollfd,EPOLL_CTL_DEL,user_data->sockfd,0);
	assert(user_data);
	char buf[38]= "long time no request,you are closed.";
	send(user_data->sockfd,buf,strlen(buf),0);
	close(user_data->sockfd);
	printf("cllose fd %d\n",user_data->sockfd);
}

int main(int argc,char *argv[])
{
	if(argc<=2)
	{
		printf("usage: %s  ip  port_number\n",basename(argv[0]));
		return 1;
	}
	const char *ip = argv[1];
	int port = atoi(argv[2]);

	struct sockaddr_in address;
	bzero(&address,sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	inet_pton(AF_INET,ip,&address.sin_addr);

	int sock = socket(AF_INET,SOCK_STREAM,0);
	assert(sock>=0);

	int ret = bind(sock,(struct sockaddr*)&address,sizeof(address));
	assert(ret!=-1);

	ret = listen(sock,5);
	assert(ret!=-1);
	
	epoll_event events[MAX_EVENT_NUMBER];
	int epollfd = epoll_create(5);
	assert(epollfd!=-1);
	addfd(epollfd,sock);

	ret = socketpair(PF_UNIX,SOCK_STREAM,0,pipefd);
	if(ret == -1)
	{
		printf("errno = %d\n",errno);

	assert(ret!=-1);
	}
	setnonblocking(pipefd[1]);
	addfd(epollfd,pipefd[0]);

	addsig(SIGTERM);
	addsig(SIGALRM);

	int stop_sever = 0;

	client_data* users = new client_data[MAX_EVENT_NUMBER];
	int timeout = 0;
	alarm(TIMESLOT);

	while(!stop_sever)
	{
		int number = epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);
		if(number<0)
		{
			if(errno == EINTR)
			{
				continue;
			}
			else
			{
				printf("errno =  %d\n",errno);
				printf("epoll_wait fail\n");
				break;
			}
		}
		int i=0;
		for(;i<number;++i)
		{
			int sockfd = events[i].data.fd;

			if(sockfd == sock)
			{
				struct sockaddr_in client;
				socklen_t len = sizeof(client);
				int connfd = accept(sock,(struct sockaddr*)&client,&len);
				addfd(epollfd,connfd);
				users[connfd].address = client;
				users[connfd].sockfd = connfd;

			    heap_timer* timer = new heap_timer(10);//**
				timer->user_data = &users[connfd];
				timer->cb_func = cb_func;
				time_t cur = time(NULL);
				timer->expire = cur +3*TIMESLOT;
				users[connfd].timer = timer;
				th.add_timer(timer);
			}
			else if((sockfd == pipefd[0])&&(events[i].events & EPOLLIN))
			{
				int sig;
				char signals[1024];
				ret = recv(pipefd[0],signals,sizeof(signals),0);
				if(ret == -1)
				{
					//handle errno
					continue;
				}
				else if(ret == 0)
				{
					continue;
				}
				else
				{
					int i=0;
					for(;i<ret;++i)
					{
						switch(signals[i])
						{
							case SIGALRM:
							{
								timeout = 1;
								break;
							}
							case SIGTERM:
							{
								stop_sever = 1;
							}
						}
					}
				}
			}
			else if(events[i].events & EPOLLIN)
			{
				memset(users[sockfd].buf,'\0',BUFFER_SIZE);
				ret = recv(sockfd,users[sockfd].buf,BUFFER_SIZE-1,0);
				heap_timer* timer = users[sockfd].timer;
				if(ret<0)
				{
					if(errno!=EAGAIN)
					{	
						cb_func(&users[sockfd]);
						if(timer)
						{
							th.del_timer(timer);
						}
					}
				}
				else if(ret == 0)
				{
					cb_func(&users[sockfd]);
					if(timer)
					{
						th.del_timer(timer);
					}
				}
				else
				{
					printf("get %d bytes of client data %s from %d\n",ret,users[sockfd].buf,sockfd);
					if(timer)
					{	
						time_t cur = time(NULL);
						timer->expire = cur +3*TIMESLOT;
						printf("adjust timer once\n");

					}
				}
			}
			else
			{
				printf("something else happened\n");
			}
		}
		if(timeout)
		{
			time_handler();
			timeout = 0;
		}
	}
	close(sock);
	close(pipefd[0]);
	close(pipefd[1]);
	delete []users;//???
	return 0;
}
