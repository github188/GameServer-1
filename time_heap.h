#ifndef _TIME_HEAP_H_
#define _TIME_HEAP_H_

#include "event.h"

#define HEAP_INIT_SIZE 32                                           //堆的初始大小

struct time_heap {
    struct event *array;                                            //堆数组
    unsigned int capacity;                                          //数组的容量
    unsigned int size;                                              //堆中元素的个数
}; 


int heap_init(struct time_heap *heap);                              //初始化堆
int heap_add(struct time_heap *heap, struct event *ent);            //往堆中添加event实例
int heap_del(struct time_heap *heap, int entId);                    //在堆中删除eventID为entId的event实例
int heap_top(struct time_heap *heap, struct event *ent);            //获取堆顶元素
int heap_pop(struct time_heap *heap);                               //删除堆顶元素
int heap_percolate_down(struct time_heap *heap, int hole);          //下虑操作
int heap_resize(struct time_heap *heap);                            //堆大小扩大一倍
int heap_free(struct time_heap *heap);                              //清理内存


#endif      //_TIME_HEAP_H_
