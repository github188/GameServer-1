#ifndef _EVENT_H_
#define _EVENT_H_

/*
 * 事件类型的宏定义
 */
#define EV_TIMER    0x01                //定时事件
#define EV_READ     0x02                //读事件
#define EV_WRITE    0x03                //写事件
#define EV_SIGNAL   0x04                //信号事件

#define USER_ENTID_MIN 5                //用户可用的事件id的最小值

struct base;

struct event {
    int entId;                          //唯一的事件ID
    struct event *next;                 //所有的event实例串成一个链表

    struct base *base;                  //event所属的base实例
    int flag;                           //evnet的事件类型

    union data{
        int fd;
        int signal;
        int time;
    }data;

#define ev_fd       data.fd
#define ev_signal   data.signal
#define ev_time     data.time

    void (*ev_callback)(void *);        //回调函数
    void *arg;                          //传递给回调函数的参数
};

extern struct base *baseList[65];                   //一个信号事件只能注册到一个base实例中。baseList[SIGNAL]就是SIGNAL所注册的base实例指针

int event_init(struct event *ent, int data, int flag, int entId, void(*callback)(void*), void *arg);                //初始化event实例
int event_register(struct event *ent, struct base *base);                                                           //在base实例中注册event实例
int event_cancel(struct event *ent, struct base *base);                                                             //在base实例中注销event实例


#endif //_EVENT_H_
