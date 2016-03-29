#ifndef _BASE_H_
#define _BASE_H_

#include "event.h"
#include "time_heap.h"

struct base {
    unsigned int num_event;                         //事件总数
    unsigned int num_active_event;                  //活动事件总数

    unsigned int num_sigevent;                      //信号事件数
    unsigned int num_ioevent;                       //io事件数
    unsigned int num_timerevent;                    //定时器事件数

    struct event *sigevent_head;                    //信号事件链表
    struct event *ioevent_head;                     //io事件链表
    struct event *timerevent_head;                  //定时器事件链表
    struct event *activevent_head;                  //活动事件链表

    struct time_heap ptime_heap;                    //时间堆

    int epollfd;                                    //内核时间表描述符
    int pipefd[2];                                  //管道描述符
};

extern struct base *baseList[65];                   //一个信号事件只能注册到一个base实例中。baseList[SIGNAL]就是SIGNAL所注册的base实例指针

int base_init(struct base *base);
int base_free(struct base *base);
int base_start(struct base *base);

/*链表操作*/
int list_push(struct event **head, struct event *ent);
int list_delete(struct event **head, int entId);
int list_find(const struct event *head, int entId, struct event *ent);
int list_free(struct event **head);

void sighandler(int sig);                           //通用信号句柄


#endif      //_BASE_H_
