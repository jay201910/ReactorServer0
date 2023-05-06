#pragma once
#include"EventLoop.h"
#include"ThreadPool.h"
#include"Channel.h"

struct TcpServer 
{
    struct EventLoop* mainLoop;
    int threadNum; //线程池中子线程个数
    struct ThreadPool* threadPool;
    struct Listener* listener;  //封装了用于监听的fd和端口
};
struct Listener
{
    int lfd;
    unsigned short port;
};

//创建并初始化TcpServer
struct TcpServer* tcpServerInit(unsigned short port, int threadNum);
//初始化监听器Listener
struct Listener* listenerInit(unsigned short port);
//启动TcpServer
void tcpServerRun(struct TcpServer* server);