#pragma once
#include"EventLoop.h"
#include"Buffer.h"
#include"Channel.h"
#include"HttpRequest.h"
#include"HttpResponse.h"

/*区分使用哪种给客户端发送数据的方法 如果定义了该宏 就会通过processWrite回调函数来给客户端发送数据，详见processRead函数，
但这种方法是一整个一起发 在文件过大的场合下不适用。如果文件太大 就可以往buffer里写一点后就发一点*/
#define MSG_SEND_AUTO 


struct TcpConnection
{
    struct EventLoop* evLoop;
    struct Channel* channel;
    struct Buffer* readBuf;
    struct Buffer* writeBuf;
    struct HttpRequest* request;
    struct HttpResponse* response;
    char name[32]; //TcpConnection的名字 方便识别
};

//创建并初始化
struct TcpConnection* tcpConnectionInit(int fd, struct EventLoop* evLoop);
//销毁资源
int tcpConnectionDestroy(void* conn);