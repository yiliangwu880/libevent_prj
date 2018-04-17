
#include "bus_client_mgr.h"
#include "channel_server.h"

using namespace Proto;
using namespace std;
using namespace wyl;

BusClientConnector::BusClientConnector()
:m_state(WAIT_REQ_CONNECT_SVR_S)
, m_ps_write_key(0)
, m_time_cnt(0)
{
	memset(&m_svr_addr, 0, sizeof(m_svr_addr));
}

BusClientConnector::~BusClientConnector()
{
	if (CONNECT_OK_S == m_state)
	{
		pm_ntf_disconnect_pc ntf;
		ntf.ps_write_key = m_ps_write_key;
		if (ClientBusMgr::Instance().m_mgr_connector.IsConnect())
		{
			ClientBusMgr::Instance().m_mgr_connector.Send(ntf);
		}
		else
		{
			LOG_ERROR("error state. bcc on CONNECT_OK_S, and mc is disconnect");
		}
	}
	ClientBusMgr::Instance().UnregBcc(this);
}

void BusClientConnector::ConnectInit(const sockaddr_in &addr)
{
	if (0 != m_svr_addr.sin_port)
	{
		LOG_ERROR("repeated init");
		return;
	}
	m_ps_write_key = 0;
	m_state = WAIT_REQ_CONNECT_SVR_S;
	m_svr_addr = addr;
	ClientBusMgr::Instance().RegBcc(this);
	if (ClientBusMgr::Instance().m_mgr_connector.IsConnect())
	{
		OnMcConnected();
	}
}

void BusClientConnector::ConnectInit(unsigned short listen_port, const char *listen_ip /*= nullptr*/)
{
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(listen_port);
	if (nullptr != listen_ip)
	{
		addr.sin_addr.s_addr = inet_addr(listen_ip);
	}
	ConnectInit(addr);
}

bool BusClientConnector::ConnectOk(const Proto::ShareMemory &sm)
{
	if (WAIT_NTF_CONNECT_SVR_S != m_state)
	{
		LOG_ERROR("error state. %d", m_state);
		return false;
	}
	if (nullptr != m_shm_queue.get())
	{
		LOG_ERROR("repeated init");
		return false;
	}
	m_shm_queue.reset(new ShmQueueMgr());
	if (!m_shm_queue->Init(sm.write_key, sm.read_key, sm.len))
	{
		LOG_ERROR("ShmQueueMgr init fail");
		m_shm_queue.reset();
		return false;
	}
	m_ps_write_key = sm.read_key; //ps write == pc read
	OnConnected();
	m_state = CONNECT_OK_S;
	return true;
}


void BusClientConnector::OnMcConnected()
{
	if (WAIT_REQ_CONNECT_SVR_S == m_state)
	{
		ReqConnect();
	}
	else if (WAIT_REQ_TRY_RECONNECT_S == m_state)
	{
		TryReqReConnect();
	}
	else
	{
		LOG_ERROR("error state %d", (int)m_state);
	}
}

void BusClientConnector::OnMcDisonnected()
{
	if (CONNECT_OK_S == m_state)
	{
		BeDisconnect();
	}
	else if (WAIT_NTF_CONNECT_SVR_S == m_state)
	{
		m_state = WAIT_REQ_CONNECT_SVR_S;
	}
	else if (WAIT_REQ_CONNECT_SVR_S)
	{
	}
	else
	{
		LOG_ERROR("unknow state = %d", m_state);
	}
}

void BusClientConnector::TryReqReConnect()
{
	if (WAIT_REQ_TRY_RECONNECT_S != m_state)
	{
		LOG_ERROR("error state %d", (int)m_state);
		return;
	}
	if (!ClientBusMgr::Instance().m_mgr_connector.IsConnect())
	{
		return;
	}
	req_connect_svr req;
	req.svr_addr = m_svr_addr;
	ClientBusMgr::Instance().m_mgr_connector.Send(req);
	m_state = WAIT_NTF_CONNECT_SVR_S;
}

void BusClientConnector::ReqConnect()
{
	if (WAIT_REQ_CONNECT_SVR_S != m_state)
	{
		LOG_ERROR("error state %d", (int)m_state);
		return;
	}
	req_connect_svr req;
	req.svr_addr = m_svr_addr;
	ClientBusMgr::Instance().m_mgr_connector.Send(req);
	m_state = WAIT_NTF_CONNECT_SVR_S;
}

void BusClientConnector::PrintShmMsg()
{
	LOG_DEBUG("reader_msg_count=%d", m_shm_queue->reader_msg_count());
	LOG_DEBUG("writer_msg_count=%d", m_shm_queue->writer_msg_count());
}
//10ms调用一次
void BusClientConnector::OnTimer()
{
	if (CONNECT_OK_S == m_state)
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
		OnRecv(msg.data, msg.len);
	}
	else if (WAIT_REQ_TRY_RECONNECT_S == m_state)//case 连接失败的情况，会进这里
	{
		if (0 == m_time_cnt%(100*10))//每10秒重连
		{
			LOG_DEBUG("try connect again. m_time_cnt=%d", m_time_cnt);
			TryReqReConnect();//重连
		}
		m_time_cnt++;
	}
	else if (WAIT_NTF_CONNECT_SVR_S)
	{
	}
	else if (WAIT_REQ_CONNECT_SVR_S)
	{
	}
	else
	{
		LOG_ERROR("unknow state %d", m_state);
	}
}

void BusClientConnector::BeDisconnect()
{
	if (CONNECT_OK_S == m_state || WAIT_NTF_CONNECT_SVR_S == m_state)
	{
	}
	else
	{
		LOG_ERROR("error state %d", m_state);
		return;
	}
	{//最后一次接受数据
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
		OnRecv(msg.data, msg.len);
	}
	m_shm_queue.reset();
	m_ps_write_key = 0;
	on_disconnected();
	m_state = WAIT_REQ_TRY_RECONNECT_S;

}

bool BusClientConnector::Send(const char* buf, size_t len)
{
	return m_shm_queue->Send(buf, len);
}
