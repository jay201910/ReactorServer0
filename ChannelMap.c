#include"ChannelMap.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>


//初始化ChannelMap
struct ChannelMap* channelMapInit(int size)
{
    struct ChannelMap* map  = malloc(sizeof(struct ChannelMap));
    map->size = size;
    map->list = (struct ChannelMap*)malloc(size * sizeof(struct Channel*));
    return map;
}

//清空ChannelMap
void ChannelMapClear(struct ChannelMap* map) 
{
    //需要释放数组中元素指向的内存资源
    //还要释放数组自己的内存
    if(map != NULL) 
    {
        for (int i=0; i<map->size; i++) 
        {
            //遍历元素 元素指向内存不为空 就释放掉
            if (map->list[i] != NULL) {
                free(map->list[i]);
            }
        }
        free(map->list);
        map->list = NULL;
    }
    map->size = 0;
}

//ChanenlMap扩容，重新分配内存空间
bool makeMapRoom(struct ChannelMap* map, int newSize, int unitSize)
{
    if (map->size < newSize) //如果本来的size就更大 就不用扩容了
    {
        int curSize = map->size;
        //每次while循环容量增加两倍 
        while (curSize < newSize) {curSize * 2;}
        //用realloc进行内存扩容 realloc返回一个指针 指向扩容后的内存地址 如果扩容失败返回null
        struct Channel** temp = realloc(map->list, curSize * unitSize);
        if (temp == NULL) return false;
        map->list = temp;
        //初始化扩容的新增的内存
        memset(map->list[map->size], 0, curSize-map->size);
        
        map->size = curSize;
    }
    return true;
}