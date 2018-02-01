/*

*/

#pragma once

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <string.h>
#include <vector>


//单件
class LibEventMgr
{
public:
	static LibEventMgr &Instance()
	{
		static LibEventMgr d;
		return d;
	}
	//使用libevent 任何功能前，必须先调用这个初始化函数
	bool Init();


	void dispatch();
	event_base *GetEventBase(){ return m_eb; };
	bool StopDispatch();
	void RegSignal(int sig_type, void(*SignalCB)(int sig_type));

private:
	LibEventMgr()
		:m_eb(nullptr)
	{
	}
	~LibEventMgr();

private:
	event_base* m_eb;
};
