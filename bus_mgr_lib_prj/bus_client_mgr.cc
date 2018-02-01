
#include "bus_client_mgr.h"
#include "channel_server.h"

using namespace Proto;
using namespace std;
using namespace wyl;


ClientBusMgr::ClientBusMgr()
:m_state(WAIT_INIT_S)
{

}

bool ClientBusMgr::Init(unsigned short bus_mgr_port)
{
	if (WAIT_INIT_S != m_state)
	{
		LOG_ERROR("repeated init?");
		return false;
	}
	if (!m_mgr_connector.ConnectInit("127.0.0.1", bus_mgr_port))
	{
		LOG_ERROR("connect mgr fail, port = %d", bus_mgr_port);
		return false;
	}
	m_timer.StartTimer(10, nullptr, true);
	m_state = WAIT_CONNECT_MGR_S;
	return true;
}

void ClientBusMgr::OnMcConnected()
{
	if (WAIT_CONNECT_MGR_S != m_state)
	{
		LOG_ERROR("error state");
		return;
	}
	LOG_DEBUG("ClientBusMgr state = CONNECT_MGR_OK_S");
	m_state = CONNECT_MGR_OK_S;
	for (const auto &v : m_addr_2_bs)
	{
		v.second->OnMcConnected();
	}
	for (const auto &v : m_addr_2_bcc)
	{
		v.second->OnMcConnected();
	}
}

void ClientBusMgr::OnMcDisonnected()
{
	if (CONNECT_MGR_OK_S == m_state )
	{
	}
	else if (WAIT_CONNECT_MGR_S == m_state)
	{
		return;
	}
	else
	{
		LOG_ERROR("error state %d", m_state);
		return;
	}
	LOG_DEBUG("ClientBusMgr OnMcDisonnected, state = WAIT_CONNECT_MGR_S");
	m_state = WAIT_CONNECT_MGR_S;
	for (const auto &v : m_addr_2_bs)
	{
		v.second->OnMcDisonnected();
	}
	for (const auto &v : m_addr_2_bcc)
	{
		v.second->OnMcDisonnected();
	}
}

bool ClientBusMgr::RegBs(BusServer *p)
{
	auto it = m_addr_2_bs.find(p->GetAddr());
	if (it != m_addr_2_bs.end())
	{
		LOG_ERROR("repeated reg BusServer");
		return false;
	}
	m_addr_2_bs[p->GetAddr()] = p;
	return true;
}

void ClientBusMgr::UnregBs(BusServer *p)
{
	auto it = m_addr_2_bs.find(p->GetAddr());
	if (it != m_addr_2_bs.end())
	{
		m_addr_2_bs.erase(it);
	}
	else
	{
		LOG_ERROR("unreg bs fail. port=%d", ntohs(p->GetAddr().sin_port));
	}
}

BusServer * ClientBusMgr::FindBs(const sockaddr_in &addr)
{
	auto it = m_addr_2_bs.find(addr);
	if (it == m_addr_2_bs.end())
	{
		return nullptr;
	}
	return it->second;
}

bool ClientBusMgr::RegBcc(BusClientConnector *p)
{
	auto it = m_addr_2_bcc.find(p->GetAddr());
	if (it != m_addr_2_bcc.end())
	{
		LOG_ERROR("repeated reg bcc, same addr");
		return false;
	}
	m_addr_2_bcc[p->GetAddr()] = p;
	return true;
}

void ClientBusMgr::UnregBcc(BusClientConnector *p)
{
	auto it = m_addr_2_bcc.find(p->GetAddr());
	if (it != m_addr_2_bcc.end())
	{
		m_addr_2_bcc.erase(it);
	}
	else
	{
		LOG_ERROR("unreg bcc fail. port=%d", ntohs(p->GetAddr().sin_port));
	}
}

BusClientConnector * ClientBusMgr::FindBcc(const sockaddr_in &addr)
{
	auto it = m_addr_2_bcc.find(addr);
	if (it == m_addr_2_bcc.end())
	{
		return nullptr;
	}
	return it->second;
}

namespace
{
	void ForeachBscTryRevData(BusServerConnector *p)
	{
		p->OnTimer();
	}
}
void ClientBusMgr::OnTimer()
{
	for (const auto &v : m_addr_2_bcc)
	{
		v.second->OnTimer();
	}
	for (const auto &v : m_addr_2_bs)
	{
		v.second->ForeachConnector(ForeachBscTryRevData);
	}
}



void ClientBusMgr::RevPsDisconnect(int ps_write_key)
{
	for (const auto &v : m_addr_2_bs)
	{
		BusServer *bs = v.second;
		if (BusServerConnector *bsc = bs->FindBsc(ps_write_key))
		{
			bsc->DoDisconnect();
			bsc->FreeSelf();
			LOG_DEBUG("try RevPsDisconnect ok");
			return;
		}
	}
	//情况1：ps主动断开，mgr再通知断开，就忽略不处理
	LOG_DEBUG("try RevPsDisconnect fail");
}


void MgrConnectorClient::on_recv(const MsgPack &msg_pack)
{
	const BaseCmd *msg = (const BaseCmd*)(msg_pack.data);
	switch (msg->cmd_id)
	{
	default:
		LOG_ERROR("unknow cmd_id=%d", msg->cmd_id);
		break;
	case res_reg_svr_cmd:
	{
		const res_reg_svr *res = (const res_reg_svr *)msg;
		if (0 != res->error_code)
		{
			LOG_ERROR("req reg svr fail, error msg:%s", res->error_msg);
			return;
		}
		LOG_DEBUG("req reg svr ok. port=%d", ntohs(res->svr_addr.sin_port));
		BusServer *bs = ClientBusMgr::Instance().FindBs(res->svr_addr);
		if (nullptr == bs)
		{
			LOG_ERROR("find bs fail, addr port=%d", ntohs(res->svr_addr.sin_port));
			return;
		}
		bs->OnMgrCreateSvrOk();
	}
		break;
	case ntf_create_ps_channel_cmd:
	{
		const ntf_create_ps_channel *res = (const ntf_create_ps_channel *)msg;
		BusServer *bs = ClientBusMgr::Instance().FindBs(res->svr_addr);
		if (nullptr == bs)
		{
			LOG_ERROR("find bs fail, addr port=%d", ntohs(res->svr_addr.sin_port));
			return;
		}
		LOG_DEBUG("mc rev ntf_create_ps_channel_cmd");
		bs->CreatePsChannel(res->shm);
	}
		break;
	case ntf_connect_svr_cmd:
	{
		const ntf_connect_svr *res = (const ntf_connect_svr *)msg;
		BusClientConnector *bcc = ClientBusMgr::Instance().FindBcc(res->svr_addr);
		if (nullptr == bcc)
		{
			LOG_ERROR("find bcc fail, addr port=%d", ntohs(res->svr_addr.sin_port));
			return;
		}
		bcc->ConnectOk(res->shm);
	}
		break;
	case ntf_disconnect_pc_cmd:
	{
		const ntf_disconnect_pc *res = (const ntf_disconnect_pc *)msg;
		BusClientConnector *bcc = ClientBusMgr::Instance().FindBcc(res->svr_addr);
		if (nullptr == bcc)
		{
			//情况1： bcc主动断开连接，MGR会回一个断开通知，忽略不处理
			LOG_DEBUG("find bcc fail, addr port=%d", ntohs(res->svr_addr.sin_port));
			return;
		}

		bcc->BeDisconnect();
		LOG_DEBUG("RevPcDisconnect, disconnect. pc addr.port=%d", ntohs(bcc->GetAddr().sin_port));
	}
		break;
	case ntf_disconnect_ps_cmd:
	{
		const ntf_disconnect_ps *res = (const ntf_disconnect_ps *)msg;
		ClientBusMgr::Instance().RevPsDisconnect(res->ps_write_key);
	}
		break;
	}//switch (msg->cmd_id)
}

void MgrConnectorClient::on_connected()
{
	ClientBusMgr::Instance().OnMcConnected();
}

void MgrConnectorClient::on_disconnected()
{
	ClientBusMgr::Instance().OnMcDisonnected();
}

ClientBusMgrTimer::ClientBusMgrTimer()
:m_cnt(0)
{
}

//10ms调用一次
void ClientBusMgrTimer::OnTimer(void *user_data)
{
	if (0 == m_cnt % 300) //每3秒调用一次
	{
		ClientBusMgr::Instance().m_mgr_connector.TryReconnect();//有需要就重连
	}
	m_cnt++;
	//尝试读所有shm
	ClientBusMgr::Instance().OnTimer();
}
