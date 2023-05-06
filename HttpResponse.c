#include"HttpResponse.h"
#include<strings.h>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include"TcpConnection.h"

#define ResHeaderSize 16  //键值对数组的大小
//创建并初始化
struct HttpResponse* httpResponseInit()
{
    struct HttpResponse* response = (struct HttpResponse*)malloc(sizeof (struct HttpResponse));
    response->headerNum = 0;
    int size = sizeof(struct ResponseHeader) * ResHeaderSize; //响应头数组的所占内存的总大小
    response->headers = (struct ResponseHeader*)malloc(size);
    response->statusCode = Unknown;
    //初始化数组
    bzero(response->headers, size);
    bzero(response->statusMsg, sizeof(response->statusMsg));
    bzero(response->statusMsg, sizeof(response->fileName));
    //函数指针
    response->sendDataFunc = NULL;

    return response;
}

//销毁
void httpResponseDestroy(struct HttpResponse* response)
{
    if(response != NULL)
    {
        free(response->headers);
        free(response);
    }
}

//添加响应头
void httpResponseAddHeader(struct HttpResponse* response, const char* key, const char* value)
{
    if (response == NULL || key == NULL || value == NULL)
    {
        return;
    }
    strcpy(response->headers[response->headerNum].key, key);
    strcpy(response->headers[response->headerNum].value, value);
    response->headerNum++;
}

//组织http响应数据
//把HttpResponse里的数据组织好 放到sendBuf里 发送到socket
void httpResponsePrepareMsg(struct HttpResponse* response, struct Buffer* sendBuf, int socket)
{
    //状态行
    char tmp[1024] = {0};
    sprintf(tmp, "HTTP/1.1 %d %s\r\n", response->statusCode, response->statusMsg);
    bufferAppendString(sendBuf, tmp);
    //响应头
    for (int i = 0; i<response->headerNum; ++i)
    {
        sprintf(tmp, "%s: %s\r\n", response->headers[i].key, response->headers[i].value);
        bufferAppendString(sendBuf, tmp);
    }
    //空行
    bufferAppendString(sendBuf, "\r\n");
#ifndef MSG_SEND_AUTO
    //发送状态行、响应头、空行
    bufferSendData(sendBuf, socket); //往buffer里放了数据之后 就发了这些数据
#endif
   

    //发送相应的资源文件
    response->sendDataFunc(response->fileName, sendBuf, socket);

}