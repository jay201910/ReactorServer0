#pragma once
#include"Buffer.h"

//定义状态码枚举
enum HttpStatusCode 
{
    Unknown,
    OK = 200,
    MovedPermanently = 301,
    MovedTemporarily = 302,
    BadRequest = 400,
    NotFound = 404
};
//定义响应头的键值对
struct ResponseHeader
{
    char key[32];
    char value[128];   //是数组不是char*指针
};
//定义一个函数指针， 用来组织要回复给客户端的数据块
typedef void (*responseBody)(const char* fileName, struct Buffer* sendBuf, int socket);

struct HttpResponse
{
    //状态行：状态码，状态描述
    enum HttpStatusCode statusCode;
    char statusMsg[128];
    //响应头：键值对
    struct ResponseHeader* headers;
    int headerNum;
    //函数指针，将指向组织响应体的函数 
    responseBody sendDataFunc;
    //http响应的文件
    char fileName[128];
};


//创建并初始化
struct HttpResponse* httpResponseInit();
//销毁
void httpResponseDestroy(struct HttpResponse* response);
//添加响应头
void httpResponseAddHeader(struct HttpResponse* response, const char* key, const char* value);
//组织http响应数据
//把response里的数据组织好 放到sendBuf里 发送到socket
void httpResponsePrepareMsg(struct HttpResponse* response, struct Buffer* sendBuf, int socket);
