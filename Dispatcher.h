#pragma once
#include"Channel.h"
#include"EventLoop.h"

struct EventLoop; //前置声明
struct Dispatcher
{
    //成员都是函数指针

    //初始化 初始化epoll、poll、select需要的数据块
    void* (*init)();
    //将待检测的fd添加到epoll等的节点上
    //通过eventloop找到对应的dispatcher和dispatcherData
    int (*add)(struct Channel* channel, struct EventLoop* evLoop);
    //删除fd
    int (*remove)(struct Channel* channel, struct EventLoop* evLoop);
    //修改fd
    int (*modify)(struct Channel* channel, struct EventLoop* evLoop);
    //事件监测，调用epoll等函数
    int (*dispatch)(struct EventLoop* evLoop, int timeout);
    //清除数据 关闭fd、释放内存
    int (*clear)(struct EventLoop* evLoop);

};
