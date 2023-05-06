#include"Dispatcher.h"
#include<sys/select.h>
#include"Channel.h"
#include<stdlib.h>

#define Max 1024  //select最多监测1024个fd

struct SelectData 
{
    fd_set readSet;
    fd_set writeSet;
};

//加上static 函数作用域只在本文件内
static void* selectInit();
static int selectAdd(struct Channel* channel, struct EventLoop* evLoop);
static int selectRemove(struct Channel* channel, struct EventLoop* evLoop);
static int selectModify(struct Channel* channel, struct EventLoop* evLoop);
static int selectDispatch(struct EventLoop* evLoop, int timeout);
static int selectClear(struct EventLoop* evLoop);

struct Dispatcher SelectDispatcher = {
    selectInit,
    selectAdd,
    selectRemove,
    selectModify,
    selectDispatch,
    selectClear,
};

//初始化selectData
static void* selectInit()
{
    struct SelectData* data = (struct SelectData*)malloc(sizeof(struct SelectData));
    
    //用FD_ZERO宏函数清空文件描述符集合
    FD_ZERO(&data->readSet);
    FD_ZERO(&data->writeSet);
    return data;
}

//将待检测的fd添加到fd_set里
static int selectAdd(struct Channel* channel, struct EventLoop* evLoop)
{
    //转成select对应的dispatcherData类型
    struct SelectData* data = (struct SelectData*)evLoop->dispatcherData;

    //select最多监测1024个 判断一下
    if (channel->fd >= Max) return -1;

    //写的fd添加到写fdset里 读的fd添加到读fdset里
    if (channel->events & ReadEvent)
    {
       FD_SET(channel->fd, &data->readSet);
    }
    if (channel->events & WriteEvent)
    {
        FD_SET(channel->fd, &data->writeSet);
    }
   
    return 0;
}

//删除
static int selectRemove(struct Channel* channel, struct EventLoop* evLoop)
{
    //转成select对应的dispatcherData类型
    struct SelectData* data = (struct SelectData*)evLoop->dispatcherData;

    //将fd从fdset里删除
    if (channel->events & ReadEvent)
    {
       FD_CLR(channel->fd, &data->readSet);
    }
    if (channel->events & WriteEvent)
    {
        FD_CLR(channel->fd, &data->writeSet);
    }
   //通过channel释放对应的TcpConnection资源
    channel->destroyCallback(channel->arg);
    return 0;
}

//修改fd的事件
static int selectModify(struct Channel* channel, struct EventLoop* evLoop)
{
    //转成select对应的dispatcherData类型
    struct SelectData* data = (struct SelectData*)evLoop->dispatcherData;
    //select最多监测1024个 判断一下
    if (channel->fd >= Max) return -1;
    
    //添加
    if (channel->events & ReadEvent)
    {
       FD_SET(channel->fd, &data->readSet);
    }
    if (channel->events & WriteEvent)
    {
        FD_SET(channel->fd, &data->writeSet);
    }
    //删除
    if (channel->events & ReadEvent)
    {
       FD_CLR(channel->fd, &data->readSet);
    }
    if (channel->events & WriteEvent)
    {
        FD_CLR(channel->fd, &data->writeSet);
    }
    
    return 0;
}

//完成select的功能
static int selectDispatch(struct EventLoop* evLoop, int timeout)
{
    //转成select对应的dispatcherData类型
    struct SelectData* data = (struct SelectData*)evLoop->dispatcherData;

    //调用select函数 看有哪些fd发生了事件
    struct timeval val;  //timeout结构体
    val.tv_sec = timeout;
    val.tv_usec = 0;
    fd_set rdtmp = data->readSet;  //备份fdset 因为select会改变fdset
    fd_set wrtmp = data->writeSet;  
    int count = select(Max, &data->readSet, &data->writeSet, NULL, &val);  
    if (count == -1)
    {
        perror("select");
        exit(0);
    }
    //遍历fdset
    for (int i=0; i<Max; ++i)
    {
        if (FD_ISSET(i, &rdtmp))
        {
            eventActivate(evLoop, i, ReadEvent);
        }
        if (FD_ISSET(i, &wrtmp))
        {
            eventActivate(evLoop, i, WriteEvent);
        }
    }
    return 0;
}

//释放dispatcherData的内存资源
static int selectClear(struct EventLoop* evLoop)
{
    //转成select对应的dispatcherData类型
    struct SelectData* data = (struct SelectData*)evLoop->dispatcherData;
    free(data); 
    return 0;
}
