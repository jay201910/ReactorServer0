#pragma once

struct Buffer
{
    char* data; //指向数据块内存
    int capacity; //数据块的容量
    int readPos; //读数据的起始位置
    int writePos; //写数据的起始位置
};

//初始化
struct Buffer* bufferInit(int size);
//销毁
void bufferDestroy(struct Buffer* buf);
//扩容
void bufferExtendRoom(struct Buffer* buffer, int size);  //size是新添加的数据的大小
//计算剩余的可写的容量
int bufferWriteableSize(struct Buffer* buffer);
//计算剩余的可读的容量
int bufferReadableSize(struct Buffer* buffer);

//写内存 两种方式：1.直接写  2.接收套接字数据
//直接写
int bufferAppendData(struct Buffer* buffer, const char* data, int size);
int bufferAppendString(struct Buffer* buffer, const char* data);  //上面那个函数的不需要size参数的优化版本
//接收套接字数据
int bufferSocketRead(struct Buffer* buffer, int fd);
//发送数据
int bufferSendData(struct Buffer* buffer, int socket);

//找到数据块中的/r/n的位置，方便取出一行http请求
char* bufferFindCRLF(struct Buffer* buffer);