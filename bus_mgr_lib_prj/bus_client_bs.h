/*
author: yiliang.wu

function:ʵ��bs
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



//bsc
class BusServerConnector
{
public:
	BusServerConnector();
	virtual ~BusServerConnector();
	bool Init(const Proto::ShareMemory &sm, const sockaddr_in &addr, BusServer *bs);
	int GetId() const { return m_ps_write_key; }
	void OnTimer();
	void PrintShmMsg();
	//����һ����
	bool Send(const char* buf, size_t len);
	//�Ͽ����ӣ��ͷ��Լ�
	//ע�⣬���ú󣬻������ͷ��Լ������治Ҫ�������Լ���.
	//���ᴥ��on_disconnected��
	bool FreeSelf();
	void DoDisconnect();
	//ÿ�ν��ն���������Ϣ��
	virtual void on_recv(const char* buf, size_t len) = 0;
	virtual void on_connected() = 0;
	virtual void on_disconnected(){};

private:
	int m_ps_write_key; //��Ϊ bsc id��
	std::unique_ptr<ShmQueueMgr> m_shm_queue;
	BusServer *m_bs;
};

//bs
class BusServer
{
	friend class BusServerConnector;
	typedef std::map<int, BusServerConnector *> Id2Bsc;
	enum State
	{
		WAIT_REQ_MGR_REG_SVR_S, //�ȷ���ע�����������
		WAIT_RES_MGR_REG_SVR_S, // ����Ӧע��������ɹ�
		WAIT_CLIENT_CONNECT_S,
	};
public:
	BusServer();
	~BusServer();
	bool Init(unsigned short listen_port, const char *listen_ip = nullptr, bool is_outer = false);
	bool Init(const sockaddr_in &addr, bool is_outer = false);

	void CreatePsChannel(const Proto::ShareMemory &sm);
	sockaddr_in GetAddr(){ return m_addr; }

	//on mc connected, ��ʼ��busServer�ɹ�Ҳ�ᴥ��
	void OnMcConnected();
	//on mc disconnect
	void OnMcDisonnected();
	BusServerConnector *FindBsc(int ps_write_key);
	void OnMgrCreateSvrOk();
	template<class CB>
	void ForeachConnector(CB cb)
	{
		for (const auto &v : m_id_2_bsc)
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
	virtual BusServerConnector *NewConnect() = 0;
	bool FreeConnect(BusServerConnector *p);

private:
	sockaddr_in m_addr;
	Id2Bsc m_id_2_bsc; //id 2 bsc, idΪProto::ShareMemory::write_key
	State m_state;
	bool m_is_outer;

};

template<class Connector>
class BusServerT : public BusServer
{
	virtual BusServerConnector *NewConnect() override
	{
		return new Connector();
	}
};


