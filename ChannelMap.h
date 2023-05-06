#pragma once 
#include<stdbool.h>

/*ChannelMap是基于数组实现的*/

struct ChannelMap
{
    //一个二级指针，指向的是struct Channel* list[]
    //即 指向一个数组，这个数组的元素是指针，指针的类型是struct Channel* 类型
    struct Channel** list;
    int size; //数组内元素个数
};

//初始化
struct ChannelMap* channelMapInit(int size);
//清空数据的函数
void ChannelMapClear(struct ChannelMap* map);
//ChanenlMap扩容，重新分配内存空间
bool makeMapRoom(struct ChannelMap* map, int newsize, int unitSize); //newSize为扩容后的元素个数，unitSize为元素所占大小
