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
#include <map>
#include "connector.h"
#include <unistd.h>
#include <arpa/inet.h>  
#include "utility/logFile.h" //目前调试用，移植删掉


class ListenerConnector;
template<class >
class Listener;

//注意，由BaseConnectorMgr管理的 ListenerConnector不用用户代码调用delete.
//需要断开连接，调用 BaseConnectorMgr::CloseConnect(uint64 id);
class BaseConnectorMgr
{
	template<class >
	friend class Listener;
	friend class ListenerConnector;
public:
	virtual ~BaseConnectorMgr();
	//马上调用delete Connector
	bool CloseConnect(uint64 id);
	ListenerConnector *FindConnect(uint64 id);
	template<class CB>
	void ForeachConnector(CB cb)
	{
		for (const auto &v : m_all_connector)
		{
			if (nullptr == v.second)
			{
				LOG_FATAL("save null ListenerConnector");
				continue;;
			}
			(*cb)(v.second);
		}
	}

private:
	virtual ListenerConnector *NewConnect() = 0; //对象必须用new构建

	ListenerConnector *CreateConnectForListener();

private:
	std::map<uint64, ListenerConnector *> m_all_connector;
};

//管理listenner 接收的connector
template<class Connect>
class ConnectorMgr : public BaseConnectorMgr
{
private:
	virtual ListenerConnector *NewConnect();

};

//Listener 创建的connector
class ListenerConnector : public BaseSvrCon
{
public:
	ListenerConnector();
	~ListenerConnector();

	bool AcceptInit(evutil_socket_t fd, struct sockaddr* sa, const sockaddr_in &svr_addr);
	void SetCnMgr(BaseConnectorMgr *mgr){ m_cn_mgr = mgr; };
	uint64 GetId() const { return m_id; }
	sockaddr_in GetSvrAddr() const{ return m_svr_addr; }
	//断开连接，释放自己
	//注意，调用后，会马上释放自己。后面不要再引用自己了.
	//不会触发on_disconnected了
	bool FreeSelf();
private:
	virtual void OnRecv(const MsgPack &msg) override = 0;
	virtual void OnConnected() override = 0;
	virtual void on_disconnected() override final; //派生类不用继承这个函数,用onDisconnected处理被动断开连接
	virtual void onDisconnected() = 0;

private:
	BaseConnectorMgr *m_cn_mgr;
	uint64 m_id;
	sockaddr_in m_svr_addr;
	bool m_ignore_free; //防多次调用FreeSelf函数用，支持 onDisconnected里面不小心写了调用FreeSelf()
};


class NoUseConnector : public ListenerConnector
{
private:
	virtual void OnRecv(const MsgPack &msg) override{};
	virtual void OnConnected() override{};
};


//类 成员只管理 m_listener m_addr， 其他链接管理交给 BaseConnectorMgr处理
//BaseConnectorMgr 实现分离出去，让Listener做更专注的底层,更简单化。具体管理器让用户选择自定义
//@para class Connect 必须为 ListenerConnector派生类？
template<class Connect = NoUseConnector>
class Listener 
{
public:
	//para cn_mgr 生存期必须比 Listener长
	Listener(BaseConnectorMgr &cn_mgr)
		:m_listener(nullptr)
		, m_default_cn_mgr()
		, m_cn_mgr(cn_mgr)
	{
		memset(&m_addr, 0, sizeof(m_addr)); 
	}
	Listener()
		:m_listener(nullptr)
		, m_default_cn_mgr()
		, m_cn_mgr(m_default_cn_mgr)
	{
		memset(&m_addr, 0, sizeof(m_addr));
	}
	~Listener();
	bool Init(unsigned short listen_port, const char *listen_ip = nullptr);
	bool Init(const sockaddr_in &addr);
	sockaddr_in GetAddr(){ return m_addr; }
private:
	static void accept_error_cb(evconnlistener* listener, void * ctx);
	static void listener_cb(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr* sa, int socklen, void* user_data);

private:
	evconnlistener *m_listener;
	ConnectorMgr<Connect> m_default_cn_mgr;
	sockaddr_in m_addr;

public:
	BaseConnectorMgr &m_cn_mgr; 

};



template<class Connect>
ListenerConnector * ConnectorMgr<Connect>::NewConnect()
{
	return new Connect;
}



template<class Connect /*= NoUseConnector*/>
bool Listener<Connect>::Init(const sockaddr_in &addr)
{
	m_addr = addr;
	m_listener = evconnlistener_new_bind(LibEventMgr::Instance().GetEventBase(), Listener::listener_cb, (void*)this, LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1, (struct sockaddr*)&addr, sizeof(addr));
	if (!m_listener)
	{
		LOG_ERROR("evconnlistener_new_bind fail, addr.port=%d", addr.sin_port);
		return false;
	}
	evconnlistener_set_error_cb(m_listener, Listener::accept_error_cb);
	return true;
}

template<class Connect>
bool Listener<Connect>::Init(unsigned short listen_port, const char *listen_ip)
{
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(listen_port);
	if (nullptr != listen_ip)
	{
		addr.sin_addr.s_addr = inet_addr(listen_ip);
	}
	return Init(addr);
}

template<class Connect>
void Listener<Connect>::accept_error_cb(evconnlistener* listener, void * ctx)
{
	int err = EVUTIL_SOCKET_ERROR();
	LOG_ERROR("Got an error %d (%s) on the listener. \n", err, evutil_socket_error_to_string(err));
	//LibEventMgr::Instance().StopDispatch();
}


//底层已经调用了accept,获得fd，才回调这个函数
template<class Connect>
void Listener<Connect>::listener_cb(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr* sa, int socklen, void* user_data)
{
	if (nullptr == user_data)
	{
		LOG_ERROR("null cb para");
		return;
	}
	Listener* pListener = (Listener*)user_data;

	ListenerConnector *clientconn = pListener->m_cn_mgr.CreateConnectForListener();
	if (nullptr == clientconn)
	{
		LOG_ERROR("init ListenerConnector fail");
		return;
	}
	clientconn->SetCnMgr(&(pListener->m_cn_mgr));
	if (!clientconn->AcceptInit(fd, sa, pListener->m_addr))
	{
		LOG_ERROR("init ListenerConnector fail");
		return;
	}
}


template<class Connect>
Listener<Connect>::~Listener()
{
	if (nullptr != m_listener)
	{
		evconnlistener_free(m_listener);
	}
}
