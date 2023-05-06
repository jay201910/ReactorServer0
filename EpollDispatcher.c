#include"Dispatcher.h"
#include<sys/epoll.h>
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>

#define Max 520

struct EpollData 
{
    int epfd;  //epoll树根节点
    struct epoll_event* events;  //epoll_wait的传出参数 存储了已就绪的文件描述符的信息
};

//加上static 函数作用域只在本文件内
static void* epollInit();
static int epollAdd(struct Channel* channel, struct EventLoop* evLoop);
static int epollRemove(struct Channel* channel, struct EventLoop* evLoop);
static int epollModify(struct Channel* channel, struct EventLoop* evLoop);
static int epollDispatch(struct EventLoop* evLoop, int timeout);
static int epollClear(struct EventLoop* evLoop);

struct Dispatcher EpollDispatcher = {
    epollInit,
    epollAdd,
    epollRemove,
    epollModify,
    epollDispatch,
    epollClear,
};

//初始化EpollData
static void* epollInit()
{
    struct EpollData* data = (struct EpollData*)malloc(sizeof(struct EpollData));
    data->epfd = epoll_create(10);
    if (data->epfd == -1) {
        perror("epoll_create");
        exit(0);
    }
    data->events = (struct epoll_event*)calloc(Max, sizeof(struct epoll_event));
    return data;
}

//完成epoll_ctl的功能 epollAdd, epollRemove, epollModify三个函数很相似 可以简化
static int epollAdd(struct Channel* channel, struct EventLoop* evLoop)
{
    //转成epoll对应的dispatcherData类型
    struct EpollData* data = (struct EpollData*) evLoop->dispatcherData;

    //向epoll树中添加节点
    //需要知道两个信息：向epoll添加哪个fd；需要epoll检测fd的什么事件。这些信息通过传过来的channel获知
    struct epoll_event ev;
    ev.data.fd = channel->fd;  
    int events = 0;  //要监测的事件 这个事件要看过来的channel中的事件是什么 然后记录到events中
    if (channel->events & ReadEvent)
    {
        events |= EPOLLIN; //epoll检测读事件
    }
    if (channel->events & WriteEvent)
    {
        events |= EPOLLOUT;
    }
    ev.events = events;
    int ret = epoll_ctl(data->epfd, EPOLL_CTL_ADD, channel->fd, &ev);  //data的fd是epfd，channel的fd是待检测的fd
    if (ret == -1) {
        perror("epoll_ctl_add");
        exit(0);
    }
    return ret;
}

static int epollRemove(struct Channel* channel, struct EventLoop* evLoop)
{
        //转成epoll对应的dispatcherData类型
    struct EpollData* data = (struct EpollData*) evLoop->dispatcherData;

    //epoll树中删除节点  可以不管事件是什么 直接删除 
    int ret = epoll_ctl(data->epfd, EPOLL_CTL_DEL, channel->fd, NULL);  //data的fd是epfd，channel的fd是待检测的fd
    if (ret == -1) {
        perror("epoll_ctl_del");
        exit(0);
    }
    //断开连接后，需要释放cfd对应的TcpConnection资源。释放的动作通过channel里注册的回调函数完成。
    channel->destroyCallback(channel->arg);
    return ret;
}

static int epollModify(struct Channel* channel, struct EventLoop* evLoop)
{
        //转成epoll对应的dispatcherData类型
    struct EpollData* data = (struct EpollData*) evLoop->dispatcherData;

    //向epoll树中添加节点
    struct epoll_event ev;
    ev.data.fd = channel->fd;  
    int events = 0;  //声明要监测的事件 这个事件要看过来的channel中的事件是什么
    if (channel->events & ReadEvent)
    {
        events |= EPOLLIN;
    }
    if (channel->events |= WriteEvent)
    {
        events |= EPOLLOUT;
    }
    ev.events = events;
    int ret = epoll_ctl(data->epfd, EPOLL_CTL_MOD, channel->fd, &ev);  //data的fd是epfd，channel的fd是待检测的fd
    if (ret == -1) {
        perror("epoll_ctl_mod");
        exit(0);
    }
    return ret;
}

//完成epoll_wait的功能
static int epollDispatch(struct EventLoop* evLoop, int timeout)
{
    //转成epoll对应的dispatcherData类型
    struct EpollData* data = (struct EpollData*) evLoop->dispatcherData;
    int count = epoll_wait(data->epfd, data->events, Max, timeout*1000);  //Max是events数组元素个数， timeout单位本来是秒
    for (int i=0; i<count; i++)
    {
        int events = data->events[i].events;
        int fd = data->events[i].data.fd;
        if (events & EPOLLERR || events & EPOLLHUP)
        {
            //对方断开了连接
            epollRemove(channel, evLoop);
            continue;
        }
        if (events & EPOLLIN) 
        {
            eventActivate(evLoop, fd, ReadEvent);
        }
        if (events & EPOLLOUT)
        {
            eventActivate(evLoop, fd, WriteEvent);
        }
    }
    return 0;
}

//释放dispatcherData的内存资源
static int epollClear(struct EventLoop* evLoop)
{
    //转成epoll对应的dispatcherData类型
    struct EpollData* data = (struct EpollData*) evLoop->dispatcherData;
    free(data->events);
    close(data->epfd);
    free(data); //data指针指向的内存也要释放掉
    return 0;
}




