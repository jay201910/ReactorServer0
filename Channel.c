#include"Channel.h"
#include<stdlib.h>


//初始化channel结构体
struct Channel* channelInit(int fd, int events, handleFunc readFunc, handleFunc writeFunc, handleFunc destoryFunc, void* arg)
{
    //先开辟内存
    struct Channel* channel = (struct Channel*)malloc(sizeof (struct Channel));
    //再赋值
    channel->arg = arg;
    channel->fd = fd;
    channel->events = events;
    channel->readCallback = readFunc;
    channel->writeCallback = writeFunc;
    channel->destroyCallback = destoryFunc;
    return channel;
}

//修改fd的写事件，检测还是不检测fd的写事件,即要不要写这个fd
void writeEventEnable(struct Channel* channel, bool flag)
{
    if(flag)
    {
        ////如果要检测写事件 追加上写事件 按位或
        channel->events |= WriteEvent; //events是枚举类型，要么是010，要么是100，WriteEvent是100
    }
    else 
    {
        //去除掉写事件  取反再按位与
        channel->events & ~WriteEvent;
    }
}

//判断是否检测fd的写事件
bool isWriteEventEnable(struct Channel* channel)
{
    return channel->events & WriteEvent; //按位与判断是否包含
}