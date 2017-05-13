#ifndef _LST_TIMER
#define _LST_TIMER
#include"./utili.h"
#include<time.h>
#define BUFFER_SIZE 64

using namespace std;
class util_timer;
class client_data
{
public:
	sockaddr_in address;
	int sockfd;
	char buf[BUFFER_SIZE];
	util_timer *timer;
};
class util_timer
{
public:
	time_t expire;
	void (*cb_func)(client_data*);
	client_data* user_data;
	util_timer *prev;
	util_timer *next;
};
class sort_time_lst
{
public:
sort_time_lst():head(NULL),tail(NULL){}
~sort_time_lst()
{
	util_timer* tmp =head;
	while(tmp)
	{
		head = tmp->next;
		delete tmp;
		tmp = head;
	}
}
void add_timer(util_timer* timer)
{
	if(!timer)
	{
		return;
	}
	if(!head)
	{
		head = tail = timer;
		return;
	}
	if(timer->expire < head->expire)
	{
		timer->next = head;
		head->prev = timer;
		head = timer;
		return;
	}
	add_timer(timer,head);
}
void adjust_timer(util_timer* timer)
{
	if(!timer)
	{
		return;
	}
	util_timer *tmp = timer->next;
	if(!tmp ||(timer->expire < tmp->expire))
	{
		return;
	}
	if(timer == head)
	{
		head = head->next;
		head->prev =NULL;
		timer->next = NULL;
		add_timer(timer,head);
	}
	else
	{
		timer->prev->next = timer->next;
		timer->next->prev = timer->prev;
		add_timer(timer,timer->next);
	}
}
void del_timer(util_timer* timer)
{
	if(!timer)
	{
		return;
	}
	if((timer == head)&&(timer == tail))
	{
		delete timer;
		head = tail =NULL;
		return;
	}
	if(timer == head)
	{
		head = head->next;
		head->prev =NULL;
		delete timer;
		return;
	}
	if(timer == tail)
	{
		tail = tail->prev;
		tail->next = NULL;
		delete timer;
		return;
	}
	timer->prev->next =timer->next;
	timer->next->prev = timer->prev;
	delete timer;
}
void tick()
{
	if(!head)
	{
		return;
	}
	printf("timer tick\n");
	time_t cur = time(NULL);
	util_timer *tmp = head;
	while(tmp)
	{
		if(cur < tmp->expire)
		{
			break;
		}
		tmp->cb_func(tmp->user_data);
		head = tmp->next;
		if(head)
		{
			head->prev = NULL;
		}
		delete tmp;
		tmp = head;
	}
}

private:

	void add_timer(util_timer* timer,util_timer* lst_head)
	{
		util_timer* prev = lst_head;
		util_timer* tmp = prev->next;
		while(tmp)
		{
			if(tmp->expire <timer->expire)
			{
				timer->next = tmp;
				timer->prev = prev;
				prev->next = timer;
				tmp->prev = timer;
				break;
			}
			prev = tmp;
			tmp = tmp->next;
		}
		if(!tmp)
		{
			prev->next = timer;
			timer->prev = prev;
			timer->next = NULL;
			tail = timer;
		}
	}
	util_timer *head;
	util_timer* tail;
};
#endif
