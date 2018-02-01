/*
author: yiliang.wu

���ܣ� ʵ��bc

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
		WAIT_REQ_CONNECT_SVR_S, //��mc����, pc ���� ps
		WAIT_NTF_CONNECT_SVR_S, //��mc��Ӧ, pc ���� ps
		CONNECT_OK_S, //
		WAIT_REQ_TRY_RECONNECT_S, //���Ͽ������,������
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
	//ÿ�ν��ն���������Ϣ��
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

