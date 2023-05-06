#include"ThreadPool.h"
#include<assert.h>
#include<stdlib.h>
#include"Log.h"

//初始化线程池
struct ThreadPool* threadPoolInit(struct EventLoop* mainLoop, int count)  //count是子线程个数
{
    struct ThreadPool* pool = (struct ThreadPool*)malloc(sizeof(struct ThreadPool));
    pool->index = 0;  //要取第index个线程
    pool->isStart = false;
    pool->mainLoop = mainLoop;
    pool->threadNum = count;
    pool->workerThreads = (struct WorkerThread*)malloc(sizeof(struct WorkerThread)*count); //只是给子线程们分配了内存空间，还没有初始化
    return pool;
}

//启动线程池  就是启动每一个子线程
void threadPoolRun(struct ThreadPool* pool)
{
    assert(pool && !pool->isStart); //pool指针有效 且 线程池未被启动才行
    if(pthread_self() != pool->mainLoop->threadID)  //判断当前线程是不是主线程
    {
        Debug("当前线程不是主线程");
        exit(0); 
    }
    pool->isStart = true;
    //启动每一个子线程
    if (pool->threadNum)  //threadNum大于0
    {
        for (int i=0; i<pool->threadNum; ++i)
        {
            workerThreadInit(&pool->workerThreads[i], i);
            workerThreadRun(&pool->workerThreads[i]);
        }
    }
}

//取出线程池中的某个子线程的反应堆实例
struct EventLoop* takeWorkerEventLoop(struct ThreadPool* pool)
{
    assert(pool->isStart); //线程池要启动了才行
    if(pthread_self() != pool->mainLoop->threadID)  //判断当前线程是不是主线程
    {
        exit(0); 
    }
    //从线程池中找一个子线程， 然后取出里边的反应堆实例
    //如果没有子线程 就用主线程自己的evLoop
    struct EventLoop* evLoop = pool->mainLoop;
    if (pool->threadNum > 0)
    {
        evLoop = pool->workerThreads[pool->index].evLoop;
        //更新index 为了不是总让同一个线程工作
        pool->index = ++pool->index % pool->threadNum; //先加一 再取余
    }
    return evLoop;
}

