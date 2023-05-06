#pragma once
#include<stdbool.h>

//定义函数指针，用于声明回调函数。参数不确定
typedef int(*handleFunc)(void* arg);

struct  Channel
{
    //有文件描述符、事件、读写回调
    int fd; 
    int events;
    handleFunc readCallback;
    handleFunc writeCallback;
    handleFunc destroyCallback; //销毁资源的回调函数 用于销毁channel所在的TcpConnection的资源
    void* arg; //回调函数的参数
};

//定义fd的events读写事件 用标志位表示
enum FDEVent
{
    ReadEvent = 0x02,  //010
    WriteEvent = 0x04,  //100
};

//初始化一个channel的函数
struct Channel* channelInit(int fd, int events, handleFunc readFunc, handleFunc writeFunc, handleFunc destoryFunc, void* arg);
//修改fd的写事件,检测还是不检测fd的写事件，即要不要写这个fd
void writeEventEnable(struct Channel* channel, bool flag);
//判断是否检测fd的写事件
bool isWriteEventEnable(struct Channel* channel);
