/*
使用例子：
class MyConnectClient1 : public BaseClientCon
{
private:
	virtual void OnRecv(const MsgPack &msg) override
	{
		LOG_DEBUG("1 OnRecv %s", &msg.data);

	}
	virtual void OnConnected() override
	{
		LOG_DEBUG("1 OnConnected, send first msg");
		MsgPack msg;
		Str2MsgPack("1 msg", msg);
		send_data(msg);
	}
};

BaseSvrCon 使用参考listener.h说明
*/

#pragma once

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <string>
#include <vector>
#include <limits>
#include "lev_mgr.h"

class BaseConnectorMgr;

const static int MAX_MSG_DATA_LEN = 1024 *4; //4k
struct MsgPack  
{
	int len; //data有效字节数
	char data[MAX_MSG_DATA_LEN];
};
template<class >
class Listener;

class IConnect
{
public:
	virtual ~IConnect() {};

	//每次接收都是完整消息包
	virtual void OnRecv(const MsgPack &msg) = 0;
	virtual void OnConnected() = 0;
	virtual void OnError(short events) {};
	//删除本对象， 不会触发on_disconnected了
	virtual void on_disconnected() = 0;


};

class BaseSvrCon;
class BaseClientCon;

//管理一个socket链接，bufferevent， fd,消息收发处理
//服务器端，客户端端都可复用的功能
//要主动关闭连接，删掉对象就可以了。
class BaseConnect
{
	template<class >
	friend class Listener;
	friend class BaseSvrCon;
	friend class BaseClientCon;
public:
	BaseConnect(IConnect &iconnect);
	virtual ~BaseConnect();
	bool IsConnect() const { return m_is_connect; };

	void DisConnect();

	bool send_data(const MsgPack &msg);
	//设置发送最大缓冲大小，超了就断开连接
	void SetMaxSendBufSize(size_t num) { m_msbs = num; }

	//参考 bufferevent_setwatermark说明
	void setwatermark(short events, unsigned int lowmark, unsigned int highmark);

	void GetRemoteAddr(std::string &ip, unsigned short &port) const;
	sockaddr_in GetRemoteAddr() const { return m_addr; }

	// true表示已经接收部分字节， 等接受完整消息包.
	bool IsWaitCompleteMsg() const;
	evutil_socket_t GetFd() { return m_fd; };
	//测试专用,无消息头,自由发送指定字节数
	bool send_data_no_head(const char* data, int len);
	void SetEventCbLog(bool no_ev_cb_log) { m_no_ev_cb_log = no_ev_cb_log; }

private:
	static void writecb(struct bufferevent* bev, void* user_data);
	static void eventcb(struct bufferevent* bev, short events, void* user_data);
	static void readcb(struct bufferevent* bev, void* user_data);

	void conn_write_callback(bufferevent* bev);//如果不用 考虑删掉，
	void conn_read_callback(bufferevent* bev);
	void conn_event_callback(bufferevent* bev, short events);

	void free();
	bool SetSocketInfo(bufferevent* buf_e, evutil_socket_t fd, struct sockaddr* sa= nullptr);
	void SetIsConnect(bool is_connect){m_is_connect = is_connect;}
	void SetAddr(const char* connect_ip, unsigned short connect_port);
	void SetAddr(const sockaddr_in &svr_addr);
private:
	static const size_t MAX_MAX_SEND_BUF_SIZE = 1024 * 64;
	bufferevent* m_buf_e;
	sockaddr_in m_addr;			//对方地址
	evutil_socket_t m_fd;		
	size_t m_msbs;				//max send buf size发送最大缓冲大小，超了就断开
	bool m_is_connect;			//true表示已经连接
	MsgPack m_msg;				//每个接收消息包。
	int m_msg_write_len;		//m_msg内存写入的字节数
	bool m_no_ev_cb_log;		//true表示不打印事件回调错误
	IConnect &m_iconnect;		//引用具体回调实现的对象
};


//管理服务器端链接， 远程端为客户端
class BaseSvrCon : public IConnect
{
public:
	BaseSvrCon():m_com(*this)
	{}
	//让Listener调用的函数，用户别调用
	//para sa 对方地址
	//para svr_addr server addr
	bool AcceptInit(evutil_socket_t fd, struct sockaddr* sa);
	bool send_data(const MsgPack &msg) {
		return m_com.send_data(msg);
	}

	bool IsConnect() const { return m_com.IsConnect(); }
public:
	BaseConnect m_com;

private:
};

//管理客户端端链接， 远程端为客户端
class BaseClientCon : public IConnect
{
public:
	BaseClientCon() :m_com(*this)
	{}
	//AConnectInit必须先选其中一个初始化函数调用后，才能使用其他方法
	//return true代表请求成功，不代表连接成功. 不能连接成功，会回调 on_disconnected  通知
	bool ConnectInit(const char* connect_ip, unsigned short connect_port);
	bool ConnectInit(const sockaddr_in &svr_addr);
	bool TryReconnect();
	bool send_data(const MsgPack &msg) {
		return m_com.send_data(msg);
	}
	bool IsConnect() const { return m_com.IsConnect(); }
private:
	bool ConnectByAddr();

public:
	BaseConnect m_com;

private:
};




