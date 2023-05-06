#define _GNU_SOURCE
#include"HttpRequest.h"
#include<stdio.h>
#include<strings.h>
#include<string.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<dirent.h>
#include<fcntl.h>
#include<unistd.h>
#include"TcpConnection.h"
#include<assert.h>
#include<ctype.h>

#define HeaderSize 12 //请求头键值对的个数

//创建并初始化
struct HttpRequest* httpRequestInit()
{
    struct HttpRequest* request = (struct HttpRequest*)malloc(sizeof(struct HttpRequest));
    request->curState = ParseReqLine;
    request->method = NULL;
    request->url = NULL;
    request->version = NULL;
    request->reqHeaders = (struct RequestHeader*)malloc(sizeof(struct RequestHeader)*HeaderSize);
    request->reqHeadersNum = 0;
    return request;
}

//重置 因为浏览器会多次发送请求 
void httpRequestResetEx(struct HttpRequest* req)
{
    //先释放指针指向的堆内存 然后将指针置空

    free(req->url);
    free(req->method);
    free(req->version);
    req->curState = ParseReqLine;
    req->method = NULL;
    req->url = NULL;
    req->version = NULL;
    
    //重置reqHeaders
    if (req->reqHeaders != NULL) 
    {
        //释放reqHeaders数组的每个元素指向的内存
        for (int i=0; i < req->reqHeadersNum; ++i) 
        {
            free(req->reqHeaders[i].key);
            free(req->reqHeaders[i].value);//key和value都是指针
        }
        free(req->reqHeaders); //释放reqHeaders数组本身
    }
    req->reqHeadersNum = 0;
}

//销毁
void httpRequestDestroy(struct HttpRequest* req)
{
    if (req != NULL)
    {
        httpRequestResetEx(req); //直接调用上面那个重置函数来销毁部分东西
        free(req);  
    }
}

//获取处理状态
enum HttpRequestState httpRequestState(struct HttpRequest* req)
{
    return req->curState;
}

//向reqHeaders里添加请求头
void httpRequestAddHeader(struct HttpRequest* request, const char* key, const char* value)
{
    request->reqHeaders[request->reqHeadersNum].key = (char*)key;
    request->reqHeaders[request->reqHeadersNum].value = (char*)value;
    request->reqHeadersNum++;
}


//通过reqHeader的key查找value
char* httpRequestGetHeader(struct HttpRequest* request, const char* key)
{
    if (request != NULL)
    {
        //遍历 逐个对比key 找到key
        for (int i=0; i < request->reqHeadersNum; ++i)
        {
            if (strncasecmp(request->reqHeaders[i].key, key, strlen(key)) == 0)
            {
                return request->reqHeaders[i].value;
            }
        }
    }
    return NULL;
}

//解析请求行。 解析readBuf里的数据，将请求行、请求头等信息赋给httpRequest。
bool parseHttpRequestLine(struct HttpRequest* request, struct Buffer* readBuf)
{
    //找到请求行结束符号的位置
    char* end = bufferFindCRLF(readBuf);
    //请求行起始位置
    char* start = readBuf->data + readBuf->readPos;
    //请求行总长度
    int lineSize = end - start;
    if (lineSize)
    {
        //请求行格式： get /xx/xxx.txt http/1.1

        //请求方式
        char* space = memmem(start, lineSize, ' ', 1);  //space空格的意思
        assert(space != NULL);
        int methodSize = space - start;
        //把请求方式的字符串赋给request->method
        request->method = (char*)malloc(methodSize+1); //先给request->method开辟内存 +1是为了在最后加一个/0
        strncpy(request->method, start, methodSize);  //给request->method赋值
        request->method[methodSize] = '\0';  //字符串末尾加\0

        //请求的资源
        start = space + 1;
        space = memmem(start, end - start, ' ', 1); 
        assert(space != NULL);
        int urlSize = space - start;
        request->url = (char*)malloc(urlSize+1); 
        strncpy(request->url, start, urlSize);
        request->url[urlSize] = '\0';

        //http版本
        start = space + 1;
        request->version = (char*)malloc(end - start + 1); 
        strncpy(request->version, start, end - start);
        request->version[end - start] = '\0';

        //为解析请求头做准备
        readBuf->readPos += lineSize;
        readBuf->readPos += 2; //跳过\r\n
        //修改状态
        request->curState = ParseReqHeaders;
        return true;
    }
}

//解析请求头 解析 一行
bool parseHttpRequestHeader(struct HttpRequest* request, struct Buffer* readBuf)
{
    char* end = bufferFindCRLF(readBuf);
    if (end != NULL)
    {
        char* start = readBuf->data + readBuf->readPos;
        int lineSize = end - start;
        //找到键值对中的： 
        char* middle = memmem(start, lineSize, ": ", 2); //注意冒号后有个空格
        if (middle != NULL)
        {
            char* key = malloc(middle - start + 1); //分配内存。加1是为了在末尾加一个\0
            strncpy(key, start, middle - start); //内存赋值
            key[middle - start] = "\0";

            char* value = malloc(end - middle -2 + 1); //加一是因为middle后有冒号空格
            strncpy(value, middle + 2, end - middle -2);
            value[end - middle -2] = "\0";

            httpRequestAddHeader(request, key, value);
            //移动数据读取的位置
            readBuf->readPos += lineSize;
            readBuf->readPos += 2;

        }
        else 
        {
            //没有找到冒号 是空行 请求头解析完了
            readBuf->readPos += 2;
            //修改解析状态
            //忽略post请求 按照get请求处理。不管请求体了，直接结束解析。
            request->curState = ParseReqDone;
        }
        return true;
    }
    return false;
}

//解析http请求协议
bool parseHttpRequest(struct HttpRequest* request, struct Buffer* readBuf, struct HttpResponse* response, struct Buffer* sendBuf, int socket)
{
    bool flag = true;
    while(request->curState != ParseReqDone)
    {
        switch (request->curState)
        {
            
        case ParseReqLine:
            //解析请求行
            flag = parseHttpRequestLine(request, readBuf);
            break;
        case ParseReqHeaders:
            //解析请求头
            flag = parseHttpRequestHeader(request, readBuf);
            //这里之后可能就结束解析了 状态为ParseReqDone
            break;
        case ParseReqBody:
            //不解析请求体
            break;  
        default:
            break;
        }

        if (!flag)
        {
            //解析失败，直接结束
            return flag;
        }

        //判断是否解析完成了 如果解析完成了 需要准备回复的数据
        if (request->curState == ParseReqDone)
        {
            //1.根据解析出的数据 设计一个HttpResponse模块
            processHttpRequest(request, response);
            //2.根据HttpResponse组织相应的响应数据并发送给客户端
            httpResponsePrepareMsg(response, sendBuf, socket);
        }
    }
    request->curState = ParseReqLine; //状态还原 保证还能继续处理下一条http请求
    return flag;
}

//处理http get请求，根据请求，设计HttpResponse
bool processHttpRequest(struct HttpRequest* request, struct HttpResponse* response)
{
    if (strcasecmp(request->method, "get") != 0)
    {
        //不是get请求就不处理
        return -1;
    }
    decodeMsg(request->url, request->url); //中文会被浏览器解析为“%B2”这种形式发送过来，所以要转回来

    // 处理客户端请求的静态资源(目录或者文件)
    char* file = NULL;
    if (strcmp(request->url, "/") == 0)  //是资源根目录
    {
        file = "./";   //前提是服务器进程要切换到资源根目录
    }
    else
    {
        file = request->url + 1;  //不是资源根目录
    }
    // 获取文件属性
    struct stat st;
    int ret = stat(file, &st);

    // 文件不存在 -- 回复404
    if (ret == -1)
    {
        //组织response
        //状态行
        strcpy(response->fileName, "404.html");
        response->statusCode = NotFound;
        strcpy(response->statusMsg, "Not Found");
        //响应头
        httpResponseAddHeader(response, "Content-type", getFileType(".html"));
        response->sendDataFunc = sendFile;
        return 0;
    }

    // 文件存在 判断文件类型
    if (S_ISDIR(st.st_mode))
    {
        //把这个目录中的内容发送给客户端
        //组织response
        //状态行
        strcpy(response->fileName, file);
        response->statusCode = OK;
        strcpy(response->statusMsg, "OK");
        //响应头
        httpResponseAddHeader(response, "Content-type", getFileType(".html"));  //发目录也是发一个html网页
        response->sendDataFunc = sendDir;
    }
    else
    {
        // 把文件的内容发送给客户端
        //组织response
        //状态行
        strcpy(response->fileName, file);
        response->statusCode = OK;
        strcpy(response->statusMsg, "OK");
        //响应头
        httpResponseAddHeader(response, "Content-type", getFileType(file));
        char tmp[12] = {0};
        sprintf(tmp, "ld", st.st_size);
        httpResponseAddHeader(response, "Content-length", tmp);
        response->sendDataFunc = sendFile;
    }
    return 0;
}




// 将字符转换为整形数
int hexToDec(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return 0;
}
// 解码
// to 存储解码之后的数据, 传出参数, from被解码的数据, 传入参数
void decodeMsg(char* to, char* from)
{
    for (; *from != '\0'; ++to, ++from)
    {
        // isxdigit -> 判断字符是不是16进制格式, 取值在 0-f
        // Linux%E5%86%85%E6%A0%B8.jpg
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
        {
            // 将16进制的数 -> 十进制 将这个数值赋值给了字符 int -> char
            // B2 == 178
            // 将3个字符, 变成了一个字符, 这个字符就是原始数据
            *to = hexToDec(from[1]) * 16 + hexToDec(from[2]);

            // 跳过 from[1] 和 from[2] 因此在当前循环中已经处理过了
            from += 2;
        }
        else
        {
            // 字符拷贝, 赋值
            *to = *from;
        }

    }
    *to = '\0';
}

const char* getFileType(const char* name)
{
    // a.jpg a.mp4 a.html
    // 自右向左查找‘.’字符, 如不存在返回NULL
    const char* dot = strrchr(name, '.');
    if (dot == NULL)
        return "text/plain; charset=utf-8";	// 纯文本
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp(dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}

void sendDir(const char* dirName, struct Buffer* sendBuf, int cfd)
{
    char buf[4096] = { 0 };
    sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirName);
    struct dirent** namelist;
    int num = scandir(dirName, &namelist, NULL, alphasort);
    for (int i = 0; i < num; ++i)
    {
        // 取出文件名 namelist 指向的是一个指针数组 struct dirent* tmp[]
        char* name = namelist[i]->d_name;
        struct stat st;
        char subPath[1024] = { 0 };
        sprintf(subPath, "%s/%s", dirName, name);
        stat(subPath, &st);
        if (S_ISDIR(st.st_mode))
        {
            // a标签 <a href="">name</a>
            sprintf(buf + strlen(buf), 
                "<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>", 
                name, name, st.st_size);
        }
        else
        {
            sprintf(buf + strlen(buf),
                "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
                name, name, st.st_size);
        }
        bufferAppendString(sendBuf, buf);
#ifndef MSG_SEND_AUTO
        bufferSendData(sendBuf, cfd); //往buffer里放了数据之后 就发了这些数据
#endif
        memset(buf, 0, sizeof(buf));
        free(namelist[i]);
    }
    sprintf(buf, "</table></body></html>");
    bufferAppendString(sendBuf, buf);
#ifndef MSG_SEND_AUTO
    bufferSendData(sendBuf, cfd); //往buffer里放了数据之后 就发了这些数据
#endif
    free(namelist);

}


void sendFile(const char* fileName, struct Buffer* sendBuf, int cfd)
{
    // 1. 打开文件
    int fd = open(fileName, O_RDONLY);
    assert(fd > 0);
#if 1
    while (1)
    {
        char buf[1024];
        int len = read(fd, buf, sizeof buf);
        if (len > 0)
        {
            bufferAppendData(sendBuf, buf, len);
#ifndef MSG_SEND_AUTO
            bufferSendData(sendBuf, cfd); //往buffer里放了数据之后 就发了这些数据
#endif
            //usleep(10); // 这非常重要
        }
        else if (len == 0)
        {
            break;
        }
        else
        {
            close(fd);
            perror("read");
        }
    }
#else
    off_t offset = 0;
    int size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    while (offset < size)
    {
        int ret = sendfile(cfd, fd, &offset, size - offset);
        printf("ret value: %d\n", ret);
        if (ret == -1 && errno == EAGAIN)
        {
            printf("没数据...\n");
        }
    }
#endif
    close(fd);

}


