#pragma once
#include"Dispatcher.h"
#include<stdbool.h>
#include"ChannelMap.h"
#include<pthread.h>
#include"Channel.h"

//此变量是在别处定义的，要在此处引用
extern struct Dispatcher EpollDispatcher;
extern struct Dispatcher PollDispatcher;
extern struct Dispatcher SelectDispatcher;

//处理该节点中的channel的方式
enum ElemType{ADD, DELETE, MODIFY};
//定义任务队列的节点
struct ChannelElement
{
    int type; //如何处理该节点中的channel 有三种类型 故用枚举类型实现
    struct Channel* channel;
    struct ChannelElement* next;
};

struct Dispatcher; //前置声明
struct EventLoop
{
    bool isQuit;  //EventLoop是否在工作中

    struct Dispatcher* dispatcher;
    void* dispatcherData;

    //任务队列 用链表实现
    struct ChannelElement* head;
    struct ChannelElement* tail;
    //channelMap
    struct ChannelMap* channelMap;
    //当前线程的id和name
    pthread_t threadID;
    char threadName[32];
    //互斥锁 用于线程同步 保护任务队列
    pthread_mutex_t mutex;

    //用于唤醒阻塞的线程
    int socketPair[2];  //存储本地通信的两个fd 通过socketPair初始化
    
};

//创建并初始化EventLoop 
//由于子线程需要指定名字 所以写两个初始化函数 一个用于初始化主线程 一个用于子线程
struct EventLoop* eventLoopInit();
struct EventLoop* eventLoopInitEx(const char* threadName);

//启动反应堆模型
int eventLoopRun(struct EventLoop* evLoop);

//处理激活的文件描述符fd
int eventActivate(struct EventLoop* evLoop, int fd, int event);

//添加任务到任务队列
int eventLoopAddTask(struct EventLoop* evLoop, struct Channel* channel, int type);

//处理任务队列中的任务
int eventLoopProcessTask(struct EventLoop* evLoop);

//处理dispatcher中的节点
int eventLoopAdd(struct EventLoop* evLoop, struct Channel* channel);
int eventLoopRemove(struct EventLoop* evLoop, struct Channel* channel);
int eventLoopModify(struct EventLoop* evLoop, struct Channel* channel);
//释放channel 回收资源
int destroyChannel(struct EventLoop* evLoop, struct Channel* channel);


