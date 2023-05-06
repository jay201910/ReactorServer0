#pragma once
#include"Buffer.h"
#include<stdbool.h>
#include"HttpResponse.h"

//解析http请求过程中的四个阶段
enum HttpRequestState
{
    //解析请求行、请求头、请求体、结束解析
    ParseReqLine,  
    ParseReqHeaders,
    ParseReqBody,
    ParseReqDone,
};

struct HttpRequest
{
    //请求行
    char* method;
    char* url;
    char* version;
    //请求头 是一个个键值对 用数组存储
    struct RequestHeader* reqHeaders;
    int reqHeadersNum; //请求头中键值对的个数
    //解析过程的状态
    enum HttpRequestState curState;
};
//请求头 键值对
struct RequestHeader
{
    char* key;
    char* value;
};



//创建并初始化
struct HttpRequest* httpRequestInit();
//重置 因为浏览器会多次发送请求 
void httpRequestResetEx(struct HttpRequest* req);
//销毁
void httpRequestDestroy(struct HttpRequest* req);
//获取处理状态
enum HttpRequestState httpRequestState(struct HttpRequest* req);
//向reqHeaders里添加请求头
void httpRequestAddHeader(struct HttpRequest* request, const char* key, const char* value);
//通过reqHeader的key查找value
char* httpRequestGetHeader(struct HttpRequest* request, const char* key);
//解析请求行
bool parseHttpRequestLine(struct HttpRequest* request, struct Buffer* readBuf);
//解析请求头
bool parseHttpRequestHeader(struct HttpRequest* request, struct Buffer* readBuf);
//解析http请求协议
bool parseHttpRequest(struct HttpRequest* request, struct Buffer* readBuf, struct HttpResponse* response, struct Buffer* sendBuf, int socket);  
//处理http请求协议，组织一个HttpResponse
bool processHttpRequest(struct HttpRequest* request, struct HttpResponse* response);
// 解码字符串
void decodeMsg(char* to, char* from);
const char* getFileType(const char* name);
void sendDir(const char* dirName, struct Buffer* sendBuf, int cfd);
void sendFile(const char* fileName, struct Buffer* sendBuf, int cfd);
