#pragma once
#include<pthread.h>
#include"EventLoop.h"


//子线程对应的结构体实体
struct WorkerThread
{
    pthread_t threadID; //线程id
    char name[24]; //线程名字
    pthread_mutex_t mutex; //互斥锁 用于线程同步
    pthread_cond_t cond; //条件变量 用于阻塞线程
    struct EventLoop* evLoop; 
};

//初始化workerThread
int workerThreadInit(struct WorkerThread* thread, int index);  //index是为了拼名字