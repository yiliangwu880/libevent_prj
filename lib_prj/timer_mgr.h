/*
use excample:
class MyTimer : public BaseLeTimer
{
private:
virtual void OnTimer(void *user_data) override{};
};

main()
{
	LibEventMgr::Instance().Init();
	MyTimer o;
	o.StartTimer(1000, &user_data);

	LibEventMgr::Instance().dispatch();
}
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
#include "lev_mgr.h"


//里面做创建，销毁定时器，保证不内存泄露, 不回调不存在的BaseTime
//注意：没怎么测试验证，建议只设几个全局的循环定时器。 一次性定时器自己利用循环定时器重写。
class BaseLeTimer
{
public:
	BaseLeTimer();
	virtual ~BaseLeTimer();

	//para is_loop true表示循环定时器
	//return, true成功开始定时器，false 已经开始了，不需要设定(可以先stop,再start)
	bool StartTimer(unsigned long long millisecond, void *para, bool is_loop = false);
	//停止正在进行的定时器，
	//return, false 不需要停止. true 成功操作了停止
	bool StopTimer();

	virtual void OnTimer(void *para) = 0;
private:
	static void TimerCB(evutil_socket_t, short, void* para);
private:
	enum State
	{
		S_WAIT_START_TIMER,
		S_WAIT_TIME_OUT,
	};
	event* m_event;
	State m_state;
	void *m_para;
};