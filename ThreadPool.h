#pragma once
#include<stdbool.h>
#include"EventLoop.h"
#include"WorkerThread.h"

struct ThreadPool
{
    struct EventLoop* mainLoop; //主线程的反应堆模型
    bool isStart; //线程池是否启动
    int threadNum; //线程池中子线程个数
    struct  WorkerThread* workerThreads; //包含子线程的数组
    int index; //所选中的子线程的序号   
};

//创建并初始化线程池
struct ThreadPool* threadPoolInit(struct EventLoop* mainLoop, int count);

//启动线程池
void threadPoolRun(struct ThreadPool* pool);

//取出线程池中的某个子线程的反应堆实例
struct EventLoop* takeWorkerEventLoop(struct ThreadPool* pool);
