#define _GNU_SOURCE
#include"Buffer.h"
#include<stdio.h>
#include<stdlib.h>
#include<sys/uio.h>
#include<string.h>
#include<unistd.h>
#include<strings.h>
#include<sys/socket.h>

//初始化
struct Buffer* bufferInit(int size)
{
    struct Buffer* buffer = (struct Buffer*)malloc(sizeof(struct Buffer));
    if (buffer != NULL)
    {
        buffer->data = (char*)malloc(size); //申请data指针指向的内存
        memset(buffer->data, 0, size); //malloc申请的内存要初始化
        buffer->capacity = size;
        buffer->readPos = buffer->writePos = 0;
    }
    return buffer;
}

//销毁
void bufferDestroy(struct Buffer* buf)
{
    if (buf != NULL)
    {
        if (buf->data != NULL) free(buf->data);
    }
    free(buf);
}

//扩容
void bufferExtendRoom(struct Buffer* buffer, int size)
{
    //要判断内存够不够用 再决定扩不扩容
    //1.内存够用 不扩容
    if (bufferWriteableSize(buffer) >= size)
    {
        return;
    }
    //2.已读过的内存+未写过的内存 是够用的，不需要扩容，将未读的内存移动到前面去 以腾出连续的空余内存
    else if (buffer->readPos + bufferWriteableSize(buffer) >= size)
    {
        //得到未读内存的大小
        int readable = bufferReadableSize(buffer);
        //移动未读的内存块
        memcpy(buffer->data, buffer->data + buffer->readPos, readable);
        //更新位置
        buffer->readPos = 0;
        buffer->writePos = readable;
    }
    //3.内存不够用 扩容 用realloc
    else
    {
        void* temp = realloc(buffer->data, buffer->capacity+size);
        if (temp == NULL) return; //扩容失败
        //初始化新开辟的内存
        memset(temp + buffer->capacity, 0, size);
        //更新
        buffer->data = temp;
        buffer->capacity += size;
    }
}
//计算剩余的未写的内存容量
int bufferWriteableSize(struct Buffer* buffer)
{
    return buffer->capacity - buffer->writePos;
}
//计算剩余的未读的内存容量
int bufferReadableSize(struct Buffer* buffer)
{
    return buffer->writePos - buffer->readPos;
}

//写内存 直接写  
int bufferAppendData(struct Buffer* buffer, const char* data, int size)
{
    if (buffer == NULL || data == NULL || data <= 0)
    {
        return -1;
    }
    //扩容
    bufferExtendRoom(buffer, size);
    //数据拷贝
    memcpy(buffer->data + buffer->writePos, data, size);
    buffer->writePos += size;
    return 0;
}
int bufferAppendString(struct Buffer* buffer, const char* data)
{
    //这个函数就是对上面一个函数的再封装。有时候调用strlen函数并不能得到字符串长度，所以上面那个函数还是需要单独存在的 
    int size = strlen(data);
    int ret = bufferAppendData(buffer, data, size);
    return ret;
}

//接收套接字数据
int bufferSocketRead(struct Buffer* buffer, int fd)
{
    //用readv函数 可以将网络通信发过来的数据接收到多个数据缓冲区中
    struct iovec vec[2];  //数组元素为iovec类型结构体，iovec就是接收数据的缓冲块，这里定义两个 
    //初始化iovec缓冲块。两个缓冲块 一个就是Buffer， 一个是又malloc申请的堆内存
    int writeable = bufferWriteableSize(buffer);
    vec[0].iov_base = buffer->data + buffer->writePos; //iov_base指向数据块的可写起始地址
    vec[0].iov_len = writeable;  //iov_len表示数据块可写的内存大小
    char* tmpbuf = (char*)malloc(40960); //给第二个iovec用的 记得要free
    vec[1].iov_base = buffer->data + buffer->writePos;
    vec[1].iov_len = 40960;
    int result = readv(fd, vec, 2); //分散读函数readv，返回值就是一共接收了多少数据
    if(result == -1)
    {
        return -1;
    }
    else if (result <= writeable)
    {
        //数据不是很多，全读到第一个iovec中去了。 
        buffer->writePos += result;  //更新writePos
    }
    else
    {
        //第一个iovec写满了，还写到了第二个iovec里。就要把第二个iovec的数据移到Buffer后面
        buffer->writePos = buffer->capacity; //更新writePos
        bufferAppendData(buffer, tmpbuf, result-writeable);
    }
    free(tmpbuf);
    return result;
}

//发送数据
int bufferSendData(struct Buffer* buffer, int socket)
{
    //判断有无待发送的数据
    int readable = bufferReadableSize(buffer);
    if (readable > 0)
    {
        int count = send(socket, buffer->data + buffer->readPos, readable, MSG_NOSIGNAL);
        if (count > 0)
        {
            buffer->readPos += count;
            usleep(1); //发慢一点 让接收端喘口气
        }
        return count;
    }
    return 0;
}


//找到数据块中的/r/n的位置，方便取出一行http请求
char* bufferFindCRLF(struct Buffer* buffer)
{
    //用memmem函数进行字符串的查找
    char* ptr = memmem(buffer->data + buffer->readPos, bufferReadableSize, "\r\n", 2);
    return ptr;
}