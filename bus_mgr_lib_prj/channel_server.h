/*
author: yiliang.wu

���ܣ�
*/

#pragma once
#include <map>
#include "bus_proto.h"
#include <arpa/inet.h>  
#include "include_all.h"
#include "bus_typedef.h"
#include "share_memory.h"
#include "channel.h"

class MgrConnector;
class OuterChannelServer;


//ocsc = outer channel server	connector
class Ocsc : public ListenerConnector
{
public:
	Ocsc();

private:
	virtual void on_recv(const MsgPack &msg) override;
	virtual void on_connected() override;
	virtual void onDisconnected() override;

	OuterChannelServer *FindChannel();

private:
	ChannelId m_channel_id;
};

//cs
class ChannelServer
{
public:
	ChannelServer();
	bool Init(uint64 mc_id, const sockaddr_in &addr, bool is_outer);
	sockaddr_in GetAddr(){ return m_addr; }
	uint64 GetMcId() const { return m_mc_id; }
	Ocsc *FindOcsc(uint64 ocsc_id);

private:
	Listener<Ocsc> m_listener;
	bool m_is_outer; //true��ʾ��������������
	sockaddr_in m_addr;
	uint64 m_mc_id;
};


//cs mgr
//channel server, ps����mgr���̵ķ�������������socket listener,���� ����(ic�����)
class ChannelServerMgr
{
	ChannelServerMgr(){};
	typedef std::map<sockaddr_in, ChannelServer *, AddrCompareLess> Addr2Svr;
public:
	static ChannelServerMgr &Instance()
	{
		static ChannelServerMgr d;
		return d;
	}
	void McRevReqRegSvr(MgrConnector &mc, const Proto::req_reg_svr *req);

	ChannelServer * FindSvr(const sockaddr_in &addr);
	template<class CB>
	void ForeachSvr(CB cb)
	{
		for (const auto &v: m_addr_2_svr)
		{
			(*cb)(v.second);
		}
	}
	void OnMcDisconnect(uint64 mc_id);

private:
	bool CreateSvr(uint64 mc_id, const sockaddr_in &addr, bool is_outer, std::string &error_msg);

private:
	Addr2Svr m_addr_2_svr;
};
