#include"TcpConnection.h"
#include"HttpRequest.h"
#include<stdlib.h>
#include<stdio.h>
#include"Log.h"

//读事件的回调函数： 将客户端发送过来的数据放到conn->readBuf中
int processRead(void* arg)
{
    struct TcpConnection* conn = (struct TcpConnection*)arg;
    //接收数据
    int count = bufferSocketRead(conn->readBuf, conn->channel->fd);

    Debug("接收到http请求数据: %s", conn->readBuf->data + conn->readBuf->readPos);

    if (count > 0)
    {
#ifdef MSG_SEND_AUTO
        //检测cfd的写事件，使用写回调函数processWrite回调函数发送数据
        writeEventEnable(conn->channel, true);
        eventLoopAddTask(conn->evLoop, conn->channel, MODIFY); //修改cfd待检测事件 从读 改为 读写
#endif
        //接收到了http请求 解析请求
        int socket = conn->channel->fd;
        bool flag = parseHttpRequest(conn->request, conn->readBuf, conn->response, conn->writeBuf, socket);
        if (!flag)
        {
            //解析失败 回复一个简单的html
            char* errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
            bufferAppendString(conn->writeBuf, errMsg);
        }   
    }
    else
    {
#ifndef MSG_SEND_AUTO
        //如果是往writeBuf里写一点数据就发一点数据，那么parseHttpRequest函数运行完后，整个http响应数据就发送完了，现在就要断开连接
        //而如果是通过写回调processWrite发送数据的话，断开连接的操作在processWrite里
        eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
#endif
    }
#ifndef MSG_SEND_AUTO
    //断开连接
    eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
#endif
    return 0;
}
//写事件的回调函数：把conn->writBuf里的数据发送给客户端
int processWrite(void* arg)
{
    Debug("开始发送数据(基于写事件发送)");
    struct TcpConnection* conn = (struct TcpConnection*)arg;    
    //发送数据
    int count = bufferSendData(conn->writeBuf, conn->channel->fd);
    if(count > 0)
    {
        //判断数据是否全部被发送出去了 是就删除这个节点
        if(bufferReadableSize(conn->writeBuf) == 0)
        {
            eventLoopAddTask(conn->evLoop, conn->channel, DELETE);
        }
    }

}

//创建并初始化
struct TcpConnection* tcpConnectionInit(int fd, struct EventLoop* evLoop)
{
    struct TcpConnection* conn = (struct TcpConnection*)malloc(sizeof(struct TcpConnection));
    conn->evLoop = evLoop;
    conn->readBuf = bufferInit(10240);
    conn->writeBuf = bufferInit(10240);
    conn->request = httpRequestInit();
    conn->response = httpResponseInit();
    sprintf(conn->name, "Connection-%d", fd);
    conn->channel = channelInit(fd, ReadEvent, processRead, processWrite, tcpConnectionDestroy, conn);
    eventLoopAddTask(evLoop, conn->channel, ADD);  //把channel添加到任务队列
    Debug("和客户端建立连接， threadName: %s, thredID: %s, connName: %s", 
        evLoop->threadName, evLoop->threadID, conn->readBuf);

    return conn;
}



//销毁资源 作为channel里回调函数调用
int tcpConnectionDestroy(void* arg)
{
    struct TcpConnection* conn = (struct TcpConnection*)arg;  
    if (conn != NULL) 
    {
        if (conn->readBuf && bufferReadableSize(conn->readBuf) == 0 &&
            conn->writeBuf && bufferReadableSize(conn->writeBuf) ==0)  //判断一下读写buf里是不是没有数据了
        {
            destroyChannel(conn->evLoop, conn->channel);
            bufferDestroy(conn->readBuf);
            bufferDestroy(conn->writeBuf);
            httpRequestDestroy(conn->request);
            httpResponseDestroy(conn->response);
            free(conn);
        }
    }
    Debug("连接断开,释放资源, connName: %s, conn->name");
    return 0;
}