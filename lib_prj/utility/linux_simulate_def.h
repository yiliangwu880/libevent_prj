/*
brief: ���� vsд������ѣ�����WINƽ̨û�е�linux����
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

#define EPOLLIN 1 //����ʾ��Ӧ���ļ����������Զ��������Զ�SOCKET�����رգ���
#define EPOLLOUT 1 //����ʾ��Ӧ���ļ�����������д��
#define EPOLLPRI 1 //����ʾ��Ӧ���ļ��������н��������ݿɶ�������Ӧ�ñ�ʾ�д������ݵ�������
#define EPOLLERR 1 //����ʾ��Ӧ���ļ���������������
#define EPOLLHUP 1 //����ʾ��Ӧ���ļ����������Ҷϣ�

#define EPOLLET 1 //�� ��EPOLL��Ϊ��Ե����(Edge Triggered)ģʽ�����������ˮƽ����(Level Triggered)��˵�ġ�
#define EPOLLONESHOT 1 //��ֻ����һ���¼���������������¼�֮���������Ҫ�����������socket�Ļ�����Ҫ�ٴΰ����socket���뵽EPOLL������

#define snprintf _snprintf_s 


inline struct tm *localtime_r(const time_t *timep, struct tm *result)
{
	localtime_s(result, timep);
	return result;
}

#endif //#ifdef WIN32
//#ifdef WIN32
