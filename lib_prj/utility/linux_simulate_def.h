/*
brief: 方便 vs写代码而已，定义WIN平台没有的linux定义
*/
#pragma once

#ifdef WIN32
#include <time.h>
typedef int __uint32_t;
typedef long long  __uint64_t;

typedef union epoll_data {  
	void *ptr;  
	int fd;  
	__uint32_t u32;  
	__uint64_t u64;  
} epoll_data_t;  

struct epoll_event {  
	__uint32_t events; /* Epoll events */  
	epoll_data_t data; /* User data variable */  
};  

#define EPOLLIN 1 //：表示对应的文件描述符可以读（包括对端SOCKET正常关闭）；
#define EPOLLOUT 1 //：表示对应的文件描述符可以写；
#define EPOLLPRI 1 //：表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）；
#define EPOLLERR 1 //：表示对应的文件描述符发生错误；
#define EPOLLHUP 1 //：表示对应的文件描述符被挂断；

#define EPOLLET 1 //： 将EPOLL设为边缘触发(Edge Triggered)模式，这是相对于水平触发(Level Triggered)来说的。
#define EPOLLONESHOT 1 //：只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个socket的话，需要再次把这个socket加入到EPOLL队列里

#define snprintf _snprintf_s 


inline struct tm *localtime_r(const time_t *timep, struct tm *result)
{
	localtime_s(result, timep);
	return result;
}

#endif //#ifdef WIN32
//#ifdef WIN32
