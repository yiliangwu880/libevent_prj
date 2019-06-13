/*
author: yiliang.wu

���ܣ� progress �� busmgr ���ӵĿͻ��ˣ�progress��������
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
#include "bus_client_bc.h"
#include "bus_client_bs.h"

class BusServer;
class BusClientConnector;

typedef std::map<sockaddr_in, BusServer *, AddrCompareLess> Addr2BS;
typedef std::map<sockaddr_in, BusClientConnector *, AddrCompareLess> Addr2BCC;


//mc = mgr connector = p ͨ��socket����mgr ��connector
class MgrConnectorClient : public BaseClientCon
{
public:
	template<class ResMsg>
	bool Send(const ResMsg &res)
	{
		MsgPack pack;
		memcpy(pack.data, &res, sizeof(res));
		pack.len = sizeof(res);
		return send_data(pack);

	}
	template<class ResMsg>
	bool SendError(ResMsg &res, const std::string &msg, uint16 error_code = 1)
	{
		res.error_code = error_code;
		memcpy(res.error_msg, msg.c_str(), msg.length() + 1);

		MsgPack pack;
		memcpy(pack.data, &res, sizeof(res));
		pack.len = sizeof(res);
		return send_data(pack);
	}
private:
	virtual void OnRecv(const MsgPack &msg_pack) override;
	virtual void OnConnected() override;
	virtual void on_disconnected();
};



class ClientBusMgrTimer : public BaseLeTimer
{
public:
	ClientBusMgrTimer();
	virtual void OnTimer(void *user_data) override;

private:
	int m_cnt;
};

class ClientBusMgr
{
	enum State
	{
		WAIT_INIT_S,
		WAIT_CONNECT_MGR_S,
		CONNECT_MGR_OK_S,
	};
public:
	static ClientBusMgr &Instance()
	{
		static ClientBusMgr d;
		return d;
	}
	ClientBusMgr();
	//para bus_mgr_port ����bus mgr�˿�
	bool Init(unsigned short bus_mgr_port);
	void OnMcConnected();
	void OnMcDisonnected();
	bool RegBs(BusServer *p);
	void UnregBs(BusServer *p);
	BusServer *FindBs(const sockaddr_in &addr);
	bool RegBcc(BusClientConnector *p); //��֧��ע��2����ͬ��ַ�Ŀͻ���
	void UnregBcc(BusClientConnector *p);
	BusClientConnector *FindBcc(const sockaddr_in &addr);
	void OnTimer();
	void RevPsDisconnect(int ps_write_key);

public:
	MgrConnectorClient m_mgr_connector;
	ClientBusMgrTimer m_timer;

private:
	Addr2BS m_addr_2_bs;
	Addr2BCC m_addr_2_bcc;
	State m_state;
};