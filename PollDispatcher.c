#include"Dispatcher.h"
#include<sys/poll.h>
#include<stdlib.h>
#include<stdio.h>


#define Max 1024  //poll的有个特点 监测的fd个数没有上限 但是个数越多 效率越低

struct PollData 
{
   int maxfd;  //要遍历的文件描述符个数 数组中最后一个有效元素的下标 这里最大为1023 0到1023正好1024个
   struct pollfd fds[Max];  //struct pollfd 类型的数组，每个struct pollfd存储了待检测的文件描述符的信息
};

//加上static 函数作用域只在本文件内
static void* pollInit();
static int pollAdd(struct Channel* channel, struct EventLoop* evLoop);
static int pollRemove(struct Channel* channel, struct EventLoop* evLoop);
static int pollModify(struct Channel* channel, struct EventLoop* evLoop);
static int pollDispatch(struct EventLoop* evLoop, int timeout);
static int pollClear(struct EventLoop* evLoop);

struct Dispatcher PollDispatcher = {
    pollInit,
    pollAdd,
    pollRemove,
    pollModify,
    pollDispatch,
    pollClear,
};

//初始化pollData
static void* pollInit()
{
    struct PollData* data = (struct PollData*)malloc(sizeof(struct PollData));
    data->maxfd = 0; 
    for(int i=0; i<Max; ++i)  //遍历struct pollfd数组
    {
        data->fds[i].fd = -1;
        data->fds[i].events = 0;
        data->fds[i].revents = 0;
    }
    return data;
};

//将待检测的fd添加到fds数组里
static int pollAdd(struct Channel* channel, struct EventLoop* evLoop)
{
    //转成poll对应的dispatcherData类型
    struct PollData* data = (struct PollData*) evLoop->dispatcherData;

    //向poll数组添加元素
    int events = 0;  //要监测的事件 这个事件要看过来的channel中的事件是什么
    if (channel->events & ReadEvent)
    {
        events |= POLLIN;
    }
    if (channel->events & WriteEvent)
    {
        events |= POLLOUT;
    }
    //遍历fds数组 找一个没有被占用的位置
    int i = 0;
    for (; i<Max; ++i)
    {
        if (data->fds[i].fd == -1) 
        {
            data->fds[i].fd = channel->fd;
            data->fds[i].events = events;
            data->maxfd = i > data->maxfd ? i : data->maxfd; //更新maxfd
            break;
        }
    }
    if (i >= Max) return -1;  //数组中没有地方放了 最大是1023
    return 0;

}

//将的fd从fds数组里删除
static int pollRemove(struct Channel* channel, struct EventLoop* evLoop)
{
    //转成poll对应的dispatcherData类型
    struct PollData* data = (struct PollData*) evLoop->dispatcherData;

    
    //遍历fds数组 找出要删除的fd
    int i = 0;
    for (; i<Max; ++i)
    {
        if (data->fds[i].fd == channel->fd) 
        {
            data->fds[i].fd = -1;
            data->fds[i].events = 0;
            data->fds[i].revents = 0;
            break;
        }
    }
    //通过channel释放对应的TcpConnection资源
    channel->destroyCallback(channel->arg);
    return 0;
}

//修改fd的事件
static int pollModify(struct Channel* channel, struct EventLoop* evLoop)
{
    //转成poll对应的dispatcherData类型
    struct PollData* data = (struct PollData*) evLoop->dispatcherData;

    
    int events = 0;  //要监测的事件 这个事件要看过来的channel中的事件是什么
    if (channel->events & ReadEvent)
    {
        events |= POLLIN;
    }
    if (channel->events |= WriteEvent)
    {
        events |= POLLOUT;
    }
    //遍历fds数组 找对应位置
    int i = 0;
    for (; i<Max; ++i)
    {
        if (data->fds[i].fd == channel->fd) 
        {           
            data->fds[i].events = events;
            break;
        }
    }
    if (i >= Max) return -1;  
    return 0;

}

//完成poll的功能
static int pollDispatch(struct EventLoop* evLoop, int timeout)
{
    //转成poll对应的dispatcherData类型
    struct PollData* data = (struct PollData*) evLoop->dispatcherData;
    //调用poll函数 看有哪些fd发生了事件
    int count = poll(data->fds, data->maxfd+1, timeout*1000);  
    if (count == -1)
    {
        perror("poll");
        exit(0);
    }
    for (int i=0; i<=data->maxfd; ++i)
    {
        if (data->fds[i].fd == -1)
        {
            continue; //没有事件发生 跳过
        }
        if (data->fds[i].revents & POLLIN) 
        {
            eventActivate(evLoop, data->fds[i].fd, ReadEvent);
        }
        if (data->fds[i].revents & POLLOUT)
        {
            eventActivate(evLoop, data->fds[i].fd, WriteEvent);
        }
    }
    return 0;
}

//释放dispatcherData的内存资源
static int pollClear(struct EventLoop* evLoop)
{
    //转成epoll对应的dispatcherData类型
    struct PollData* data = (struct PollData*) evLoop->dispatcherData;

    free(data); //data指针指向的内存也要释放掉
    return 0;
}
