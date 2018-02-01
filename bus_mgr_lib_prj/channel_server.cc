#include "channel.h"
#include "channel_server.h"
#include "include_all.h"
#include "bus_mgr.h"
#include "bus_proto.h"

using namespace Proto;

ChannelServer::ChannelServer()
:m_listener()
,m_is_outer(false)
, m_mc_id(0)
{
	memset(&m_addr, 0, sizeof(m_addr));
}

bool ChannelServer::Init(uint64 mc_id, const sockaddr_in &addr, bool is_outer)
{
	m_mc_id = mc_id;
	m_is_outer = is_outer;
	m_addr = addr;
	if (!m_is_outer)
	{
		return true;
	}
	return m_listener.Init(addr);
}

Ocsc * ChannelServer::FindOcsc(uint64 ocsc_id)
{
	ListenerConnector *p = m_listener.m_cn_mgr.FindConnect(ocsc_id);
	return dynamic_cast<Ocsc *>(p);
}

bool ChannelServerMgr::CreateSvr(uint64 mc_id, const sockaddr_in &addr, bool is_outer, std::string &error_msg)
{
	auto it = m_addr_2_svr.find(addr);
	if (it != m_addr_2_svr.end())
	{
		LOG_DEBUG("repeated CreateSvr port=%d", ntohs(addr.sin_port));
		error_msg = "repeated req svr";
		return false;
	}
	ChannelServer *p = new ChannelServer();
	if (!p->Init(mc_id, addr, is_outer))
	{
		error_msg = "socket bind addr fail.";
		delete p;
		return false;
	}
	m_addr_2_svr[addr] = p;
	return true;
}

ChannelServer * ChannelServerMgr::FindSvr(const sockaddr_in &addr)
{
	auto it = m_addr_2_svr.find(addr);
	if (it == m_addr_2_svr.end())
	{
		return nullptr;
	}
	return it->second;
}

void ChannelServerMgr::OnMcDisconnect(uint64 mc_id)
{
	for (auto it = m_addr_2_svr.begin(); it != m_addr_2_svr.end();)
	{
		ChannelServer *p = it->second;
		if (p->GetMcId() == mc_id)
		{
			delete p;
			m_addr_2_svr.erase(it++);
		}
		else {
			++it;
		}
	}
}


void ChannelServerMgr::McRevReqRegSvr(MgrConnector &mc, const req_reg_svr *req)
{
	res_reg_svr res;
	res.svr_addr = req->svr_addr;
	std::string error_msg;
	bool ret = CreateSvr(mc.GetId(), req->svr_addr, req->is_outer, error_msg);

	if (!ret)
	{
		mc.SendError(res, error_msg);
		return;
	}
	mc.Send(res);
	LOG_DEBUG("rev req_reg_svr_channel_cmd, create cs ok");
}

void Ocsc::on_recv(const MsgPack &msg)
{
	OuterChannelServer *channel = FindChannel();
	if (nullptr == channel)
	{
		LOG_ERROR("FindChannel fail");
		return;
	}
	if (nullptr == channel->m_shm_queue.get())
	{
		LOG_ERROR("no m_shm_queue");
		return;
	}
	channel->m_shm_queue->Send(msg.data, msg.len);
}

void Ocsc::on_connected()
{
	//连接成功，创建channel, channel info 通知 p.
	ChannelServer  *cs = ChannelServerMgr::Instance().FindSvr(GetSvrAddr());
	if (nullptr == cs)
	{
		LOG_ERROR("find svr fail");
		return;
	}

	OuterChannelServer *channel = ChannelMgr::Instance().CreateOCS(GetId(), cs->GetMcId(), GetSvrAddr());
	if (nullptr == channel)
	{
		LOG_ERROR("nullptr == channel");
		FreeSelf();
		return;
	}
	m_channel_id = channel->GetId();
}


void Ocsc::onDisconnected()
{
	//里面可能会调用  ocsc FreeSelf.会自动忽略操作
	ChannelMgr::Instance().FreeChannel(m_channel_id);
}

Ocsc::Ocsc()
:m_channel_id(0)
{

}


OuterChannelServer * Ocsc::FindChannel()
{
	if (0 == m_channel_id)
	{
		LOG_ERROR("0 == m_channel_id , channel havn't create");
		return nullptr;
	}
	BaseChannel *p = ChannelMgr::Instance().FindChannel(m_channel_id);
	OuterChannelServer *r = dynamic_cast<OuterChannelServer *>(p);
	if (nullptr == r)
	{
		LOG_ERROR("get OuterChannelServer have error type.");
	}
	return r;
}
