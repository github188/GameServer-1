#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include "base.h"

#define MAX_EVENTS 1024

struct base *baseList[65];                   //一个信号事件只能注册到一个base实例中。baseList[SIGNAL]就是SIGNAL所注册的base实例指针

/*
 * 函数名称：base_init
 * 函数功能：初始化base结构体；
 * 参数：base指针，传递初始化对象
 * 返回值：成功返回0，否则返回-1
 */
int base_init(struct base *base)
{
    if (!base)
        return -1;

    base->num_event = 0;
    base->num_active_event = 0;
    base->num_sigevent = 0;
    base->num_ioevent = 0;
    base->num_timerevent = 0;

    base->sigevent_head = NULL;
    base->ioevent_head = NULL;
    base->timerevent_head = NULL;
    base->activevent_head = NULL;

    heap_init(&base->ptime_heap);

    if (pipe(base->pipefd) < 0)
        return -1;
    if (fcntl(base->pipefd[0], F_SETFL, fcntl(base->pipefd[0], F_GETFL) | O_NONBLOCK) < 0)
        return -1;
    
    base->epollfd = epoll_create(5);
    if (base->epollfd < 0)
        return -1;

    struct epoll_event epollEnt;
    epollEnt.events = EPOLLIN | EPOLLET;
    epollEnt.data.u32 = 1;
    if (epoll_ctl(base->epollfd, EPOLL_CTL_ADD, base->pipefd[0], &epollEnt) < 0)
        return -1;

    return 0;
}


/*
 * 函数名称：base_free
 * 函数功能：程序结束时释放base所占的资源
 * 参数：base指针，传递释放对象
 * 返回值：成功返回0，否则返回-1
 */
int base_free(struct base *base)
{
    if (base == NULL)
        return -1;

    int ret = 0;

    if (!base->sigevent_head)
        ret = list_free(&base->sigevent_head);
    if (!base->ioevent_head)
        ret = list_free(&base->ioevent_head);
    if (!base->timerevent_head)
        ret = list_free(&base->timerevent_head);
    if (!base->activevent_head)
        ret = list_free(&base->activevent_head);

    ret = heap_free(&base->ptime_heap);
    close(base->pipefd[0]);
    close(base->pipefd[1]);
    
    if (ret < 0)
        return -1;

    return 0;
}


/*
 * 函数名称：base_start
 * 函数功能：开启主循环，进行事件监听
 * 参数：base指针，传递base实例
 * 返回值：正常退出返回0，错误返回错误号(<0)
 */
int base_start(struct base *base)
{
    struct epoll_event events[MAX_EVENTS];
    time_t nowTime;
    int waitTime = 0;
    int ret = 0;
    struct event timeEnt;
    
    while(1) {
        nowTime = time(NULL);
        if (base->ptime_heap.size > 0) {
            heap_top(&base->ptime_heap, &timeEnt);
            waitTime = timeEnt.ev_time - nowTime;
            waitTime = waitTime <= 0?0:waitTime;
        }
        else
            waitTime = -1;
        ret = epoll_wait(base->epollfd, events, MAX_EVENTS, waitTime < 0?-1:waitTime * 1000);               
        if (ret < 0) {
            if (errno == EAGAIN || errno == EINTR)
                continue;
            else
                break;
        }
       
        int t = 0;
        while (base->ptime_heap.size > 0 && t <= 0) {                                                   //处理定时器事件
            nowTime = time(NULL);
            heap_top(&base->ptime_heap, &timeEnt);
            t = timeEnt.ev_time - nowTime;
            if (t <= 0) {
                heap_pop(&base->ptime_heap);
                list_delete(&base->timerevent_head, timeEnt.entId);
                base->num_timerevent--;
                list_push(&base->activevent_head, &timeEnt);
                base->num_active_event++;
            }
        }
       
        for (int i = 0; i < ret; i++) {
            int entId = events[i].data.u32;
            if (entId == 1 && events[i].events & EPOLLIN) {                                             //处理信号事件
                int sig;
                int r = read(base->pipefd[0], &sig, sizeof(sig));
                if (r == 4) {
                    struct event *pFind = base->sigevent_head;
                    while (pFind && pFind->ev_signal != sig)
                        pFind = pFind->next;
                    if (pFind) {
                        list_push(&base->activevent_head, pFind);
                        base->num_active_event++;
                    }
                }
            }
            else if (events[i].events & EPOLLIN || events[i].events & EPOLLOUT) {                       //处理IO事件
                struct event ioEnt;
                if (list_find(base->ioevent_head, entId, &ioEnt) < 0) 
                    return -1;
                list_push(&base->activevent_head, &ioEnt);
                base->num_active_event++;
            }
        }

        while (base->activevent_head) {
            struct event *p = base->activevent_head;
            p->ev_callback(p->arg);
            base->activevent_head = base->activevent_head->next;
            base->num_active_event--;
            free(p);
        }
    }
    return 0;
}


/*
 * 函数名称：list_push
 * 函数功能：将ent指向的event实例插入head指向的链表的尾部
 * 参数：head,指向"要插入链表的头节点指针"的指针；ent，要插入的event实例
 * 返回值：插入成功返回0，失败返回-1
 */
int list_push(struct event **head, struct event *ent)
{
    if (head == NULL)
        return -1;
    if (list_find(*head, ent->entId, NULL) == 0)            //entId不得重复
        return -1;

    struct event *event = (struct event *)malloc(sizeof(struct event));
    memcpy(event ,ent, sizeof(struct event));

    event->next = NULL;
    if (*head == NULL) {
        *head = event;
    }
    else {
        struct event *pFind = *head;
        while (pFind->next)
            pFind = pFind->next;
        pFind->next = event;
    }
    return 0;
}


/*
 * 函数名称：list_delete
 * 函数功能：从head指向的的链表中删除事件id为entId的event实例
 * 参数：head,指向“要删除链表的头节点指针”的指针；entId，要被删除的event实例的id
 * 返回值：删除成功返回0，失败返回-1
 */
int list_delete(struct event **head, int entId)
{
    if (head == NULL)   return -1;
    if (*head == NULL)  return -1;

    struct event *pPre = NULL;
    struct event *pFind = *head;
    
    while (pFind && pFind->entId != entId) {
        pPre = pFind;
        pFind = pFind->next;
    }
    if (!pFind)
        return -1;
    if (!pPre)
        *head = (*head)->next;
    else
        pPre->next = pFind->next;
    free(pFind);
    return 0;
}


/*
 * 函数名称：list_find
 * 函数功能：从head指向的链表中查找事件id为entid的event实例
 * 参数：head,指向要查找链表的头节点的指针；entId，要查找的event实例的id；ent,用来返回查找结果,如果只想知道id为entId的event实例是否存在ent可为NULL
 * 返回值：查找成功返回0，否则返回-1表示没找到
 */
int list_find(const struct event *head, int entId, struct event *ent)
{
    if (head == NULL) return -1;

    const struct event *pFind = head;
    while (pFind && pFind->entId != entId)
        pFind = pFind->next;
    if (pFind == NULL)
        return -1;
    if (ent != NULL)
        memcpy(ent, pFind, sizeof(struct event));
    return 0;
}


/*
 * 函数名称：list_free
 * 函数功能：释放head指向的链表所占的资源
 * 参数：head,指向要释放资源的链表头节点指针的指针
 * 返回值：成功释放返回0，否则返回错误代码
 */
int list_free(struct event **head)
{
    if (head == NULL) return -1;
    if (!*head)
        return 0;

    struct event *pFind = *head;
    struct event *t;
    while(pFind) {
        t = pFind;
        pFind = pFind->next;
        free(t);
    }

    return 0;
}


/*
 * 函数功能：通用信号处理函数,信号发生事由操作系统调用
 * 函数参数：sig,发生的信号
 */
void sighandler(int sig)
{
    int saveErrno = errno;
    do {
        struct base *base = baseList[sig];
        if (!base)
           break;
        
        int msg = sig;
        write(base->pipefd[1], &msg, sizeof(int));
    }while(0);
    errno = saveErrno;
}
