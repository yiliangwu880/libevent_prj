
#include "bus_client_mgr.h"
#include "channel_server.h"

using namespace Proto;
using namespace std;
using namespace wyl;


BusServer::BusServer()
:m_state(WAIT_REQ_MGR_REG_SVR_S)
, m_is_outer(false)
{
	memset(&m_addr, 0, sizeof(m_addr));
}

BusServer::~BusServer()
{
	ClientBusMgr::Instance().UnregBs(this);
}

bool BusServer::Init(const sockaddr_in &addr, bool is_outer)
{
	if (0 != m_addr.sin_port)
	{
		LOG_ERROR("repeated Init");
		return false;
	}
	m_addr = addr;
	m_is_outer = is_outer;
	if (!ClientBusMgr::Instance().RegBs(this))
	{
		return false;
	}
	m_state = WAIT_REQ_MGR_REG_SVR_S;
	if (ClientBusMgr::Instance().m_mgr_connector.IsConnect())
	{
		OnMcConnected();
	}
	return true;
}



bool BusServer::Init(unsigned short listen_port, const char *listen_ip /*= nullptr*/, bool is_outer /*= false*/)
{
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(listen_port);
	if (nullptr != listen_ip)
	{
		addr.sin_addr.s_addr = inet_addr(listen_ip);
	}
	return Init(addr, is_outer);
}

//mgr 响应 ntf_create_svr_channel ,创建连接
void BusServer::CreatePsChannel(const ShareMemory &sm)
{
	if (WAIT_CLIENT_CONNECT_S != m_state)
	{
		LOG_ERROR("error state %d", (int)m_state);
		return;
	}
	BusServerConnector *bsc = NewConnect();

	if (!bsc->Init(sm, m_addr, this))
	{
		LOG_ERROR("init ListenerConnector fail");
		delete bsc;
		return;
	}
	m_id_2_bsc[bsc->GetId()] = bsc;
}

void BusServer::OnMcDisonnected()
{
	if (WAIT_RES_MGR_REG_SVR_S == m_state)
	{
		LOG_DEBUG("bs on mc disconnect. change WAIT_RES_MGR_REG_SVR_S to WAIT_REQ_MGR_REG_SVR");
	}
	else if (WAIT_CLIENT_CONNECT_S == m_state)
	{
		for (const auto &v : m_id_2_bsc)
		{
			v.second->DoDisconnect();
			delete v.second;
		}
		m_id_2_bsc.clear();
		LOG_DEBUG("bs on mc disconnect. change WAIT_CLIENT_CONNECT_S to WAIT_REQ_MGR_REG_SVR");
	}
	else
	{
		LOG_ERROR("unknow state. %d", (int)m_state);
		return;
	}
	m_state = WAIT_REQ_MGR_REG_SVR_S;
}

void BusServer::OnMcConnected()
{
	if (WAIT_REQ_MGR_REG_SVR_S != m_state)
	{
		return;
	}
	req_reg_svr req;
	req.svr_addr = m_addr;
	req.is_outer = m_is_outer;
	ClientBusMgr::Instance().m_mgr_connector.Send(req);
	m_state = WAIT_RES_MGR_REG_SVR_S;
	LOG_DEBUG("bs on mc connected, req reg svr. port=%d", ntohs(m_addr.sin_port));
}


BusServerConnector * BusServer::FindBsc(int ps_write_key)
{
	auto it = m_id_2_bsc.find(ps_write_key);
	if (it == m_id_2_bsc.end())
	{
		return nullptr;
	}
	return it->second;
}

void BusServer::OnMgrCreateSvrOk()
{
	if (WAIT_RES_MGR_REG_SVR_S != m_state)
	{
		LOG_ERROR("error state %d", (int)m_state);
		return;
	}
	m_state = WAIT_CLIENT_CONNECT_S;
}

bool BusServer::FreeConnect(BusServerConnector *p)
{
	if (nullptr == p)
	{
		LOG_ERROR("nullptr == p");
		return false;
	}
	auto it = m_id_2_bsc.find(p->GetId());
	if (it == m_id_2_bsc.end())
	{
		LOG_FATAL("FreeConnect, find bsc fail. id=%llu", p->GetId());
		return false;
	}

	//excute
	m_id_2_bsc.erase(it);
	delete p;
	return true;
}


BusServerConnector::BusServerConnector()
:m_ps_write_key(0)
, m_bs(nullptr)
{

}

BusServerConnector::~BusServerConnector()
{
}

bool BusServerConnector::Init(const Proto::ShareMemory &sm, const sockaddr_in &addr, BusServer *bs)
{
	if (nullptr != m_shm_queue.get())
	{
		LOG_ERROR("repeated init");
		return false;
	}
	m_ps_write_key = sm.write_key;
	m_shm_queue.reset(new ShmQueueMgr());
	if (!m_shm_queue->Init(sm.write_key, sm.read_key, sm.len))
	{
		LOG_ERROR("m_shm_queue->Init fail");
		m_shm_queue.reset();
		return false;
	}
	on_connected();
	return true;
}

void BusServerConnector::PrintShmMsg()
{
	LOG_DEBUG("reader_msg_count=%d", m_shm_queue->reader_msg_count());
	LOG_DEBUG("writer_msg_count=%d", m_shm_queue->writer_msg_count());
}

void BusServerConnector::OnTimer()
{
	if (0 == m_shm_queue->reader_msg_count())
	{
		return;
	}
	MsgPack msg;
	size_t out_len = 0;
	if (!m_shm_queue->Recv(msg.data, wyl::ArrayLen(msg.data), out_len))
	{
		LOG_ERROR("Recv error");
		return;
	}
	msg.len = (int)out_len;
	on_recv(msg.data, msg.len);
}

bool BusServerConnector::Send(const char* buf, size_t len)
{
	return m_shm_queue->Send(buf, len);
}

bool BusServerConnector::FreeSelf()
{
	return m_bs->FreeConnect(this);
}

void BusServerConnector::DoDisconnect()
{
	if (0 == m_shm_queue->reader_msg_count())
	{
		return;
	}
	MsgPack msg;
	size_t out_len = 0;
	if (!m_shm_queue->Recv(msg.data, wyl::ArrayLen(msg.data), out_len))
	{
		LOG_ERROR("Recv error");
		return;
	}
	msg.len = (int)out_len;
	on_recv(msg.data, msg.len);

	on_disconnected();
}
