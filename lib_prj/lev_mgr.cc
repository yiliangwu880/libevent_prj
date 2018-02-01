
#include "lev_mgr.h"
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>  
#include "connector.h"

#include "utility/config.h" //目前调试用，移植删掉

using namespace std;

#define EVENT_LOG_DEBUG 0
#define EVENT_LOG_MSG 1
#define EVENT_LOG_WARN 2
#define EVENT_LOG_ERR 3


namespace
{
	//注意：回调函数中进行LibEvent 的函数调用是不安全的
	void LIB_EVENT_LOG(int severity, const char* msg)
	{
		switch (severity)
		{
		case _EVENT_LOG_DEBUG:
			LOG_DEBUG("lib event log: %s\n", msg);
			break;
		case _EVENT_LOG_MSG:
			LOG_INFO("lib event log: %s\n", msg);
			break;
		case _EVENT_LOG_WARN:
			LOG_WARN("lib event log: %s\n", msg);
			break;
		case _EVENT_LOG_ERR:
			LOG_ERROR("lib event log: %s\n", msg);
			break;
		default:
			LOG_FATAL("lib event unknow log: %s\n", msg);
			break; /* never reached*/
		}
	}
	void EVENT_FATAL_CB(int err)
	{
		LOG_FATAL("EVENT_FATAL_CB %d", err);
		exit(1);
	}

}
bool LibEventMgr::Init()
{
	if (nullptr != m_eb)
	{
		LOG_ERROR("repeated init");
		return false;
	}
	event_set_log_callback(LIB_EVENT_LOG); //lib库日志输出
	event_set_fatal_callback(EVENT_FATAL_CB);
	//LOG_DEBUG("libevent version:%s", event_get_version());
	m_eb = event_base_new();
	if (!m_eb)
	{
		LOG_ERROR("cannot event_base_new libevent ...\n");
		return false;
	}
	return true;
}

void LibEventMgr::dispatch()
{
	if (!m_eb)
	{
		LOG_ERROR("LibEventMgr not init\n");
		return;
	}
	event_base_dispatch(m_eb);
}

bool LibEventMgr::StopDispatch()
{
	if (!m_eb)
	{
		LOG_ERROR("LibEventMgr not init\n");
		return false;
	}

	return event_base_loopexit(m_eb, NULL) == 0;//如果event_base 当前正在执行激活事件的回调, 它将在执行完当前正在处理的事件后立即退出.
}

namespace
{
	void signal_cb(evutil_socket_t sig, short events, void* user_data)
	{
		if (user_data)
		{
			typedef  void(*FUN_TYPE)(int sig_type);
			FUN_TYPE fun = (FUN_TYPE)user_data;
			fun((int)sig);
		}
	}
}

void LibEventMgr::RegSignal(int sig_type, void(*SignalCB)(int sig_type))
{
	event* signal_event = evsignal_new(m_eb, sig_type, signal_cb, (void*)SignalCB);
	if (!signal_event || event_add(signal_event, NULL) < 0)
	{
		LOG_ERROR("call evsignal_new fail");
		return;
	}
}

LibEventMgr::~LibEventMgr()
{
	if (nullptr != m_eb)
	{
		event_base_free(m_eb);
	}
}
