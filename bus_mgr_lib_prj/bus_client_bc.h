/*
author: yiliang.wu

功能： 实现bc

*/

#pragma once
#include "include_all.h"
#include <map>
#include "channel.h"
#include "utility/misc.h"
#include <memory>
#include "bus_typedef.h"
#include "share_memory.h"
#include "bus_proto.h"

class BusServer;
class BusClientConnector;
class ClientBusMgr;


//bcc
class BusClientConnector
{
	friend class MgrConnectorClient;
	enum State
	{
		WAIT_REQ_CONNECT_SVR_S, //等mc请求, pc 连接 ps
		WAIT_NTF_CONNECT_SVR_S, //等mc响应, pc 连接 ps
		CONNECT_OK_S, //
		WAIT_REQ_TRY_RECONNECT_S, //被断开的情况,等重连
	};
public:
	BusClientConnector();
	virtual ~BusClientConnector();

	void ConnectInit(unsigned short listen_port, const char *listen_ip = nullptr);
	void ConnectInit(const sockaddr_in &addr);
	void OnMcConnected();
	void OnMcDisonnected();
	void OnTimer();
	void PrintShmMsg();
	bool Send(const char* buf, size_t len);
	int GetPsWriteKey() const { return m_ps_write_key; }
	sockaddr_in GetAddr() const { return m_svr_addr; }
	virtual void on_disconnected(){};
	//每次接收都是完整消息包
	virtual void on_recv(const char* buf, size_t len) = 0;
	virtual void on_connected() = 0;
	bool ConnectOk(const Proto::ShareMemory &sm);
private:
	void BeDisconnect();
	void ReqConnect();
	void TryReqReConnect();

private:
	std::unique_ptr<ShmQueueMgr> m_shm_queue;
	State m_state;
	sockaddr_in m_svr_addr;
	int m_ps_write_key; //ps write shm key
	int m_time_cnt;
};

