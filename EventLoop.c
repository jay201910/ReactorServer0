#include"EventLoop.h"
#include<assert.h>
#include"Channel.h"
#include <sys/socket.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>


//初始化EventLoop
struct EventLoop* eventLoopInit()
{
    return eventLoopInitEx(NULL);
}

//往socketPair[0]里写数据
void taskWakeup(struct EventLoop* evLoop)
{
    const char* msg = "yeah!";
    write(evLoop->socketPair[0], msg, strlen(msg));
}
//在socketPair[1]里读数据
int readLocalMessage(void* arg)
{
    struct EventLoop* evLoop = (struct EventLoop*) arg;
    char buf[256];
    read(evLoop->socketPair[1], buf, sizeof(buf));
}

struct EventLoop* eventLoopInitEx(const char* threadName)
{
    //先给struct开辟内存
    struct EventLoop* evLoop = (struct EventLoop*)malloc(sizeof (struct EventLoop));

    evLoop->isQuit = false;
    evLoop->threadID = pthread_self();
    pthread_mutex_init(&evLoop->mutex, NULL);
    //主线程用固定名字 子线程要指定名字 
    strcpy(evLoop->threadName, threadName == NULL ? "MainThread" : threadName);  //数组不能赋值 故用strcpy函数
    evLoop->dispatcher = &EpollDispatcher;  //使用epoll
    evLoop->dispatcherData = evLoop->dispatcher->init();
    //任务队列 链表
    evLoop->head = evLoop->tail = NULL;
    //map
    evLoop->channelMap = channelMapInit(128);
    
    /*用于唤醒阻塞的线程*/
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, evLoop->socketPair);  //函数成功调用后 socketPair里就有两个用于读写的fd,可以进行本地数据通信
    if (ret == -1) 
    {
        perror("socketPair");
        exit(0);
    }
    //指定evLoop->socketPiar[0]发送数据， evLoop->sokcetPair[1]接收数据
    struct Channel* channel = channelInit(evLoop->socketPair[1], ReadEvent, readLocalMessage, NULL, NULL, evLoop);
    eventLoopAddTask(evLoop, channel, ADD);

    return evLoop;
}

//启动反应堆模型
int eventLoopRun(struct EventLoop* evLoop)
{
    assert(evLoop != NULL);  //断言 判断一下指针是否为空
    //判断线程id是否正常
    if (evLoop->threadID != pthread_self()) return -1;
    //取出事件分发和检测模型
    struct Dispatcher* dispatcher = evLoop->dispatcher;
    //循环进行事件处理
    while (!evLoop->isQuit) 
    {
        dispatcher->dispatch(evLoop, 2);  //调用dispatcher的dispatch进行事件检测
        eventLoopProcessTask(evLoop); //在上一行dispatch的时候可能会阻塞，当阻塞被唤醒后，要处理一下任务队列中新添加的任务
    }

    return 0;
}

//处理激活的文件描述符fd，即调用事件触发的回调函数
int eventActivate(struct EventLoop* evLoop, int fd, int event)
{
    //传进来的fd是dispatch检测的被激活的fd，传进来的event是检测的被触发的事件
    //先判断函数传入参数是否有效
    if (fd < 0 || evLoop == NULL) return -1;
    //如何处理该fd的操作封装在channel中， channel通过channelMap找到
    struct Channel* channel = evLoop->channelMap->list[fd];  //取出fd对应的channel
    assert(channel->fd == fd);  //看取出的channel是否确实是我们要找的

    //判断被触发的是哪种事件 并调用相应的回调函数
    if (event & ReadEvent && channel->readCallback)
    {
        channel->readCallback(channel->arg);
    }
    if (event & WriteEvent && channel->writeCallback)
    {
        channel->writeCallback(channel->arg);
    }
    return 0;
}


//添加任务到任务队列
int eventLoopAddTask(struct EventLoop* evLoop, struct Channel* channel, int type)
{
    //任务队列会被多个线程同时访问 所以要加锁 保护共享资源
    pthread_mutex_lock(&evLoop->mutex);
    
    //创建任务队列的新节点
    struct ChannelElement* node = (struct ChannelElement*)malloc(sizeof (struct ChannelElement));
    node->channel = channel;
    node->type = type;
    node->next = NULL;
    //新节点插入任务队列
    if (evLoop->head == NULL) 
    {
        evLoop->head = evLoop->tail = node;
    }
    else 
    {
        evLoop->tail->next = node;  //添加到尾部
        evLoop->tail = node;  //尾部后移
    }
    pthread_mutex_unlock(&evLoop->mutex);

    //处理节点
    /*
    细节：1.对于链表节点的添加，可能是当前子线程发起的 也可能是主线程发起的
            1）修改fd的事件 一定是当前子线程发起 当前子线程完成
            2）添加新的fd 是主线程发起的（建立新连接）
         2.主线程只用于建立新连接，不能让主线程处理任务队列，需要由当前子线程处理
         所以要判断当前线程是子线程还是主线程
    */
   if (evLoop->threadID == pthread_self())
   {
    //是当前子线程添加了任务 就当前线程自己处理任务
    eventLoopProcessTask(evLoop);
   }
   else 
   {
    //是主线程添加了 要告诉子线程处理任务队列中的任务
    //但是子线程此时可能有两种状态：1.在工作  2.被阻塞 （select、poll、epoll）
    //如何解除子线程的阻塞 ：在select、poll、epoll、检测的文件描述符中添加一个新的fd，再往这个fd里写数据
    //主线程要唤醒子线程
    taskWakeup(evLoop);
   }

    return 0;
}

int eventLoopProcessTask(struct EventLoop* evLoop)
{
    pthread_mutex_lock(&evLoop->mutex);

    //遍历任务队列
    struct ChannelElement* head = evLoop->head;
    while (head != NULL)
    {
        struct Channel* channel = head->channel;
        if(head->type == ADD) 
        {
            //添加 
            eventLoopAdd(evLoop, channel);
        }
        else if(head->type == DELETE)
        {
            //删除
            eventLoopRemove(evLoop, channel);
        }
        else if(head->type == MODIFY)
        {
            //修改
            eventLoopModify(evLoop, channel);
        }
        struct ChannelElement* tmp = head; //遍历完了的节点要free掉
        head = head->next; //头节点后移
        free(tmp);
    }
    evLoop->head = evLoop->tail = NULL;

    pthread_mutex_unlock(&evLoop->mutex);

    return 0;
}

//处理dispatcher中的节点
//将channel中的fd添加到dispatcher检测的fds中
int eventLoopAdd(struct EventLoop* evLoop, struct Channel* channel)
{
    int fd = channel->fd;
    //要将fd-channel键值对添加到evLoop的channelMap中
    struct ChannelMap* channelMap = evLoop->channelMap;
    if (fd >= channelMap->size)
    {
        //Map空间不足 就需要扩容
        if (!makeMapRoom(channelMap, fd, sizeof(struct Channel*))) return -1;
    }
    //找到位置放入fd-channel
    if (channelMap->list[fd] == NULL) 
    {
        channelMap->list[fd] = channel;
        evLoop->dispatcher->add(channel, evLoop);
    }
    return 0;
}
//将channel的fd从dispatcher检测的fds中删除
int eventLoopRemove(struct EventLoop* evLoop, struct Channel* channel)
{
    int fd = channel->fd;
    struct ChannelMap* channelMap = evLoop->channelMap;
    if (fd >= channelMap->size)
    {
        //本来就不在map里 
        return -1;
    }
    int ret = evLoop->dispatcher->remove(channel, evLoop);
    return ret;
}
//释放channel 回收资源
int destroyChannel(struct EventLoop* evLoop, struct Channel* channel)
{
    //删除fd与channel的对应关系
    evLoop->channelMap->list[channel->fd] = NULL;
    //关闭fd
    close(channel->fd);
    //释放channel内存资源
    free(channel);
    return 0;
}
int eventLoopModify(struct EventLoop* evLoop, struct Channel* channel)
//修改
{
    int fd = channel->fd;
    struct ChannelMap* channelMap = evLoop->channelMap;
    if (fd >= channelMap->size || channelMap->list[fd] == NULL)
    {
        //本来就不在map里 
        return -1;
    }
    int ret = evLoop->dispatcher->modify(channel, evLoop);
    return ret;
}