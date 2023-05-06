#include"WorkerThread.h"
#include<pthread.h>
#include<stdio.h>

//初始化workerThread
int workerThreadInit(struct WorkerThread* thread, int index)
{
    thread->evLoop = NULL;
    thread->threadID = 0;
    sprintf(thread->name, "SubThread-%d", index);
    pthread_mutex_init(&thread->mutex, NULL);
    pthread_cond_init(&thread->cond, NULL);
    return 0;
}


//子线程的回调函数
void* subThreadRunning(void* arg)
{
    struct WorkerThread* thread = (struct WorkerThread*)arg; //类型转换

    //初始化线程的evLoop
    //主线程和子线程都会访问子线程的evLoop， evLoop是共享资源 需要加锁
    pthread_mutex_lock(&thread->mutex);
    thread->evLoop = eventLoopInitEx(thread->name);  //初始化子线程的evLoop
    pthread_mutex_unlock(&thread->mutex);
    pthread_cond_signal(&thread->cond);  //evLoop成功初始化了，唤醒条件变量 解除主线程的阻塞

    eventLoopRun(thread->evLoop); //启动evLoop
    return NULL;
}
//启动子线程（主线程调用的）
void workerThreadRun(struct WorkerThread* thread)
{
    //创建子线程。 线程函数就是创建、运行evLoop
    pthread_create(&thread->threadID, NULL, subThreadRunning, thread);  //这一行子线程就运行去了，主线程继续向下运行，主线程也不知道主线程运行到什么地方了
    //由于主线程要往子线程的evLoop里放任务，主线程又不知道子线程的evLoop有没有创建好，所以要用条件变量。
    //阻塞主线程 好让子线程完成evLoop的创建
    pthread_mutex_lock(&thread->mutex);
    while(thread->evLoop == NULL)
    {
        pthread_cond_wait(&thread->cond, &thread->mutex);
    }
    pthread_mutex_unlock(&thread->mutex);
}

