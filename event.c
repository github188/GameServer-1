#include "event.h"
#include "base.h"
#include "time_heap.h"
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <signal.h>
#include <fcntl.h>


/*
 * 函数名称：event_init
 * 函数功能：初始化event实例
 * 函数参数：ent,指向要初始化的event实例；data,视情况而定的数据(fd,sig,time)；flag,事件的类型；callback,事件的回调函数；arg,回调函数的指针
 * 返回值：初始化成功返回0，否则返回-1
 */
int event_init(struct event *ent, int data, int flag, int entId, void(*callback)(void*), void *arg)
{
    if (flag == EV_TIMER)
        ent->ev_time = data;
    else if (flag == EV_READ || flag == EV_WRITE)
        ent->ev_fd = data;
    else if (flag == EV_SIGNAL)
        ent->ev_signal = data;
    else
        return -1;

    if (entId < 5)
        return -1;

    ent->flag = flag;
    ent->entId = entId;
    ent->ev_callback = callback;
    ent->arg = arg;
    ent->next = NULL;

    return 0;
}


/*
 * 函数名称：event_register
 * 函数功能：在base实例中注册event实例
 * 函数参数：ent,指向要注册的event实例；base,指向被注册的base实例
 * 返回值：注册成功返回0，否则返回-1
 */
int event_register(struct event *ent, struct base *base)
{
    if (!ent || !base)
        return -1;

    int ret;
    ent->base = base;
    if (ent->flag == EV_TIMER) {                                        //定时事件注册
        if (heap_add(&base->ptime_heap, ent) < 0)
            return -1;
        if (list_push(&(base->timerevent_head), ent) < 0)
            return -1;

        base->num_timerevent++;
    }
    else if (ent->flag == EV_READ || ent->flag == EV_WRITE) {           //io事件注册
        if (fcntl(ent->ev_fd, F_SETFL, fcntl(ent->ev_fd, F_GETFL) | O_NONBLOCK) < 0)
            return -1;
        
        struct epoll_event epollEnt;
        if (ent->flag == EV_READ)
            epollEnt.events = EPOLLIN | EPOLLET;
        else
            epollEnt.events = EPOLLOUT | EPOLLET;
        epollEnt.data.u32 = ent->entId;
        
        if (epoll_ctl(base->epollfd, EPOLL_CTL_ADD, ent->ev_fd, &epollEnt) < 0)
            return -1;
        
        if (list_push(&(base->ioevent_head), ent) < 0)
            return -1;
        base->num_ioevent++;
    }
    else if (ent->flag == EV_SIGNAL) {                                  //信号事件注册
        struct sigaction sa;
        bzero(&sa, sizeof(sa));
        sa.sa_handler = sighandler;
        sa.sa_flags = SA_RESTART;
        sigemptyset(&sa.sa_mask);
        
        if (sigaction(ent->ev_signal, &sa, NULL) < 0)
            return -1;
        
        if (baseList[ent->ev_signal])                                   //一个信号事件只能注册到一个base实例且只能注册一次
            return -1;
        if (list_push(&(base->sigevent_head), ent) < 0)
            return -1;
        
        base->num_sigevent++;
        baseList[ent->ev_signal] = base;
    }
    else
        return -1;

    base->num_event++;
    return 0;
}


/*
 * 函数名称：event_cancel
 * 函数功能：在base实例中注销event实例
 * 函数参数：ent,event实例，base,base实例
 * 返回值：成功注销返回0，错误返回-1
 */
int event_cancel(struct event *ent, struct base *base)
{
    if (!ent || !base)
        return -1;

    if (ent->flag == EV_TIMER) {                                        //删除定时器事件
        if (heap_del(&base->ptime_heap, ent->entId) < 0)
            return -1;
        if (list_delete(&base->timerevent_head, ent->entId) < 0)
            return -1;

        base->num_timerevent--;
    }
    else if (ent->flag == EV_READ || ent->flag == EV_WRITE) {           //删除IO事件
        if (epoll_ctl(base->epollfd, EPOLL_CTL_DEL, ent->ev_fd, NULL) < 0)
            return -1;
        if (list_delete(&base->ioevent_head, ent->entId) < 0)
            return -1;
         
        base->num_ioevent--;
    }
    else if (ent->flag == EV_SIGNAL)                                    //删除信号事件
    {
        struct sigaction sa;
        bzero(&sa, sizeof(sa));
        sa.sa_handler = SIG_DFL;

        if (sigaction(ent->ev_signal, &sa, NULL) < 0)
            return -1;
        if (list_delete(&base->sigevent_head, ent->entId) < 0)
            return -1;

        base->num_sigevent--;
    }
    else
        return -1;

    base->num_event--;
    return 0;
}
