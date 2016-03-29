#include "time_heap.h"
#include "base.h"
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/*
 * 函数名称：heap_init
 * 函数功能：初始化堆
 * 函数参数：heap,初始化heap指向的堆
 * 返回值：成功返回0，错误返回-1
 */
int heap_init(struct time_heap *heap)
{
    if (heap == NULL)
        return -1;

    heap->array = (struct event*)malloc(sizeof(struct time_heap) * HEAP_INIT_SIZE);
    if (!heap->array)
        return -1;
    heap->size = 0;
    heap->capacity = HEAP_INIT_SIZE;
    bzero(heap->array, sizeof(struct time_heap) * HEAP_INIT_SIZE);

    return 0;
}

/*
 * 函数名称：heap_add
 * 函数功能：添加一个event实例到堆中
 * 函数参数：heap,要被添加的堆；ent,要添加的event实例
 * 返回值：成功返回0，错误返回-1
 */
int heap_add(struct time_heap *heap, struct event *ent)
{
    if (!heap || !ent)
        return -1;
    if (ent->flag != EV_TIMER)
        return -1;

    for(int i = 0; i < heap->size; i++) {
        if (heap->array[i].entId == ent->entId)                         //防止重复插入
            return -1;
    }

    if (heap->size >= heap->capacity)                                   //如果大小不够就要进行扩容操作
        heap_resize(heap);

    int hole = heap->size++;                                            //先赋值再自增
    int parent = 0;
    for (; hole > 0; hole = parent) {
        parent = (hole - 1) / 2;
        if (heap->array[parent].ev_time <= ent->ev_time)
            break;
        heap->array[hole] = heap->array[parent];
    }
    heap->array[hole] = *ent;
}


/*
 * 函数名称：heap_del
 * 函数功能：删除事件id为entId的event实例
 * 函数参数：heap,要操作的堆；entId,要删除的event实例的id
 * 返回值：成功返回0，错误返回-1
 */
int heap_del(struct time_heap *heap, int entId)
{
    if (!heap)
        return -1;

    int delIndex = -1;
    for (int i = 0; i < heap->size; i++) {
        if (heap->array[i].entId == entId) {
            delIndex = i;
            break;
        }
    }
    if (delIndex == -1)
        return -1;

    heap->array[delIndex] = heap->array[--heap->size];
    heap_percolate_down(heap, delIndex);

    return 0;
}


/*
 * 函数名称：heap_top
 * 函数功能：获得堆顶的event实例
 * 函数参数：heap,要操作的堆；ent,用于返回堆顶event实例
 * 返回值：成功返回0，错误返回-1
 */
int heap_top(struct time_heap *heap, struct event *ent)
{
    if (!heap || !ent)
        return -1;
    if (heap->size == 0)
        return -1;

    *ent = heap->array[0];
    return 0;
}


/*
 * 函数名称：heap_pop
 * 函数功能：删除堆顶event实例
 * 函数参数：heap,要操作的堆
 * 返回值：成功返回0，错误返回-1
 */
int heap_pop(struct time_heap *heap)
{
    if (!heap)
        return -1;
    if (heap->size == 0)
        return -1;

    heap->array[0] = heap->array[--heap->size];
    if (heap_percolate_down(heap, 0) < 0)
        return -1;
}


/*
 * 函数名称：heap_percolate_down
 * 函数功能：最小堆下虑操作，确保以第hole个节点为根节点的子树拥有最小堆新增
 * 函数参数：heap,要操作的堆；hole,
 * 返回值：成功返回0，错误返回-1
 */
int heap_percolate_down(struct time_heap *heap, int hole)
{
    if (!hole || hole < 0)
        return -1;

    struct event tmp = heap->array[hole];
    int child = 0;
    for (; (hole*2 + 1) <= (heap->size - 1); hole = child) {
        child = hole*2 + 1;
        if (child < heap->size-1 && heap->array[child+1].ev_time < heap->array[child].ev_time)
            child++;
        if (heap->array[child].ev_time < tmp.ev_time)
            heap->array[hole] = heap->array[child];
        else
            break;
    }
    heap->array[hole] = tmp;
}


/*
 * 函数功能：将堆数组扩大一倍
 */
int heap_resize(struct time_heap *heap)
{
    if (!heap)
        return -1;

    struct event *newArray = (struct event*)malloc(sizeof(struct event) * heap->capacity * 2);
    if (!newArray)
        return -1;
    bzero(newArray, sizeof(struct event) * heap->capacity * 2);
    memcpy(newArray, heap->array, sizeof(struct event) * heap->capacity * 2);
    free(heap->array);
    heap->array = newArray;
    heap->capacity = heap->capacity * 2;

    return 0;
}


/*
 * 函数功能：销毁堆所占的资源
 */
int heap_free(struct time_heap *heap)
{
    if (!heap)
        return -1;

    free(heap->array);
    heap->array = NULL;
    heap->capacity = 0;
    heap->size = 0;

    return 0;
}
