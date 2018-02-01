#include "channel.h"
#include "channel_server.h"
#include "share_memory.h"
#include "listener.h"
#include "bus_mgr.h"
#include "bus_proto.h"
#include <sys/types.h>
#include <ifaddrs.h>

using namespace Proto;
using namespace std;

ChannelMgr::ChannelMgr()
:m_shm_key_seed(SHM_DEFAULT_START_KEY)
{

}

ChannelMgr::~ChannelMgr()
{
	for (const auto &v : m_id_2_channel)
	{
		delete v.second;
	}
	m_id_2_channel.clear();
}

BaseChannel * ChannelMgr::FindChannel(ChannelId id)
{
	auto it = m_id_2_channel.find(id);
	if (it == m_id_2_channel.end())
	{
		return nullptr;
	}
	return it->second;
}


namespace
{
	//return true表示是本机ip
	bool IsMyIp(const string &ip)
	{
		struct ifaddrs * ifAddrStruct = NULL;
		getifaddrs(&ifAddrStruct);

		while (ifAddrStruct != NULL)
		{
			if (ifAddrStruct->ifa_addr->sa_family == AF_INET)
			{   // check it is IP4  
				// is a valid IP4 Address  
				void * tmpAddrPtr = NULL;
				tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
				char addressBuffer[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
				if (ip == addressBuffer)
				{
					return true;
				}
				//printf("%s IPV4 Address %s\n", ifAddrStruct->ifa_name, addressBuffer);
			}
			else if (ifAddrStruct->ifa_addr->sa_family == AF_INET6)
			{   // check it is IP6  
				// is a valid IP6 Address  
				void * tmpAddrPtr = NULL;
				tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
				char addressBuffer[INET6_ADDRSTRLEN];
				inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
				if (ip == addressBuffer)
				{
					return true;
				}
				//printf("%s IPV6 Address %s\n", ifAddrStruct->ifa_name, addressBuffer);
			}
			ifAddrStruct = ifAddrStruct->ifa_next;
		}
		return false;
	}
}

void ChannelMgr::McRevReqConnectSvr(uint64 mc_id, const sockaddr_in &svr_addr)
{
	string ip = inet_ntoa(svr_addr.sin_addr);
	if (IsMyIp(ip))//case is inner server, create channel immediately
	{
		LOG_DEBUG("mc rev connect,  CreateInnerChannel");
		CreateInnerChannel(mc_id, svr_addr);
	}
	else//case svr_addr is outer
	{
		LOG_DEBUG("mc rev connect,  CreateOCC");
		CreateOCC(mc_id, svr_addr);
	}
}

bool  ChannelMgr::CreateOCC(uint64 mc_id, const sockaddr_in &svr_addr)
{
	OuterChannelClient *p = new OuterChannelClient();
	if (!p->Init(mc_id, svr_addr))
	{
		delete p;
		return false;
	}
	if (!p->Connect())
	{
		delete p;
		return false;
	}

	if (!m_id_2_channel.insert(make_pair(p->GetId(), p)).second)
	{
		LOG_ERROR("m_id_2_channel.insert fail, key=%d", p->GetId());
		delete p;
		return false;
	}
	return true;
}


bool ChannelMgr::FreeChannel(ChannelId id)
{
	auto it = m_id_2_channel.find(id);
	if (it == m_id_2_channel.end())
	{
		return false;
	}

	it->second->OnDestoryChannel();
	delete it->second;
	m_id_2_channel.erase(it);
	return true;
}

OuterChannelServer *  ChannelMgr::CreateOCS(uint64 ocsc_id, uint64 mc_id, const sockaddr_in &svr_addr)
{
	OuterChannelServer *p = new OuterChannelServer();
	p->Init(ocsc_id, mc_id, svr_addr);

	if (!m_id_2_channel.insert(make_pair(p->GetId(), p)).second)
	{
		LOG_ERROR("m_id_2_channel.insert fail, key=%d", p->GetId());
		delete p;
		return nullptr;
	}
	return p;
}

void ChannelMgr::CreateInnerChannel(uint64 mc_id, const sockaddr_in &svr_addr)
{
	MgrConnector *pc_mc = BusMgr::Instance().FindMc(mc_id);
	if (nullptr == pc_mc)
	{
		LOG_ERROR("find pc_mc fail, id=%llu", mc_id);
		return;
	}
	ntf_disconnect_pc ntf;
	ntf.svr_addr = svr_addr;
	ChannelServer *cs = ChannelServerMgr::Instance().FindSvr(svr_addr);
	if (nullptr == cs)
	{
		LOG_DEBUG("ps havn't reg");
		pc_mc->Send(ntf);
		return;
	}
	MgrConnector *ps_mc = BusMgr::Instance().FindMc(cs->GetMcId());
	if (nullptr == ps_mc)
	{
		LOG_ERROR("find ps_mc fail, id=%llu", cs->GetMcId());
		pc_mc->Send(ntf);
		return;
	}
	
	InnerChannel *channel = new InnerChannel();
	if (!channel->Init(ps_mc->GetId(), pc_mc->GetId(), svr_addr))
	{
		LOG_ERROR("CreateInnerChannel fail");
		pc_mc->Send(ntf);
		delete channel;
		return;
	}
	if (!m_id_2_channel.insert(make_pair(channel->GetId(), channel)).second)
	{
		LOG_ERROR("m_id_2_channel.insert fail, key=%d", channel->GetId());
		delete channel;
		return;
	}
	
	{//notify ps
		ntf_create_ps_channel res;
		res.svr_addr = svr_addr;
		channel->GetShareShmForPS(res.shm);
		ps_mc->Send(res);
		LOG_DEBUG("ic create shm, notify ps");
	}
	//notify pc
	//地址反过来给对对方进程读写
	{
		ntf_connect_svr res;
		res.svr_addr = svr_addr;
		channel->GetShareShmForPC(res.shm);
		pc_mc->Send(res);
		LOG_DEBUG("ic create shm, notify pc");
	}
}

int ChannelMgr::CreateShmKey()
{
	LOG_DEBUG("create shm key, now seed=%d", m_shm_key_seed);
	return m_shm_key_seed++;
}

void ChannelMgr::ForeachAll(ForeachChanelCB cb)
{
	for (const auto &v : m_id_2_channel)
	{
		(*cb)(v.second);
	}
}

void ChannelMgr::OnMcDisconnect(uint64 mc_id)
{
	for (auto it = m_id_2_channel.begin(); it != m_id_2_channel.end();) 
	{
		BaseChannel *p = it->second;
		if (p->GetMcId() == mc_id)
		{
			p->OnDestoryChannel();
			delete p;
			m_id_2_channel.erase(it++);
		}
		else {
			++it;
		}
	}
}

BaseChannel::BaseChannel()
:m_mc_id(0)
{

}

BaseChannel::~BaseChannel()
{
	m_shm_s_w.remove();
	m_shm_s_r.remove();
}

bool BaseChannel::Init(uint64 mc_id, const sockaddr_in &svr_addr)
{
	if (0 != GetId())
	{
		LOG_ERROR("repeated Init");
	}

	bool is_success = false;
	for (int i = 0; i < 100; ++i)
	{
		int key = ChannelMgr::Instance().CreateShmKey();
		if (m_shm_s_w.Create(key, SHM_MAX_LEN))
		{
			is_success = true;
			break;
		}
		else
		{
			LOG_DEBUG("try create shm by key=%d, fail, use next key try again.", key);
		}
	}
	if (!is_success)
	{
		LOG_ERROR("create shm fail");
		return false;
	}
	is_success = false;
	for (int i = 0; i < 100; ++i)
	{
		int key = ChannelMgr::Instance().CreateShmKey();
		if (m_shm_s_r.Create(key, SHM_MAX_LEN))
		{
			is_success = true;
			break;
		}
		else
		{
			LOG_DEBUG("try create shm by key=%d, fail, use next key try again.", key);
		}
	}
	if (!is_success)
	{
		LOG_ERROR("create shm fail");
		return false;
	}

	m_mc_id = mc_id;
	m_svr_addr = svr_addr;
	return true;
}


MgrConnector * BaseChannel::GetMc()
{
	return BusMgr::Instance().FindMc(GetMcId());
}

ChannelId BaseChannel::GetId() const
{
	return m_shm_s_w.get_key();
}

void BaseChannel::GetShareShmForPS(Proto::ShareMemory &sm)const
{
	sm.len = m_shm_s_w.get_size();
	sm.write_key = m_shm_s_w.get_key();
	sm.read_key = m_shm_s_r.get_key();
}

void BaseChannel::GetShareShmForPC(Proto::ShareMemory &sm) const
{
	sm.len = m_shm_s_r.get_size();
	sm.write_key = m_shm_s_r.get_key();
	sm.read_key = m_shm_s_w.get_key();
}

OuterChannelClient::OuterChannelClient()
:m_occc(*this)
{

}

bool OuterChannelClient::Connect()
{
	return m_occc.ConnectInit(GetSvrAddr());
}

void OuterChannelClient::OnConnected()
{
	if (nullptr != m_shm_queue.get())
	{
		LOG_ERROR("repeated create shm");
		return;
	}
	MgrConnector *mc = BusMgr::Instance().FindMc(GetMcId());
	if (nullptr == mc)
	{
		LOG_ERROR("find mc fail, id=%llu", GetMcId());
		return;
	}

	shmem &sw = GetShmPsW();
	shmem &sr = GetShmPsR();

	ShmQueueMgr *sqm = new ShmQueueMgr();
	if (!sqm->Init(sw.get_key(), sr.get_key(), sr.get_size()))
	{
		LOG_ERROR("open shm fail. ");
		delete sqm;
		return;
	}
	m_shm_queue.reset(sqm);

	//通知 pc ,地址反过来给对对方进程读写
	ntf_connect_svr res;
	GetShareShmForPC(res.shm);
	mc->Send(res);
	LOG_DEBUG("Occc create shm, notify pc");
}

void OuterChannelClient::TrySendData()
{
	if (nullptr != m_shm_queue.get())
	{
		LOG_ERROR("no shm");
		return;
	}
	if (0 == m_shm_queue->reader_msg_count())
	{
		return;
	}
	MsgPack msg;
	size_t out_len = 0;
	if (!m_shm_queue->Recv(msg.data, wyl::ArrayLen(msg.data), out_len))
	{
		return;
	}
	msg.len = (unsigned short)out_len;
	m_occc.send_data(msg);
}

void OuterChannelClient::OnDestoryChannel()
{
	MgrConnector * pc_mc = GetMc();
	if (nullptr == pc_mc)
	{
		return;
	}

	ntf_disconnect_pc ntf;
	ntf.svr_addr = GetSvrAddr();
	pc_mc->Send(ntf);
}

Occc::Occc(OuterChannelClient &channel)
:m_owner_channel(channel)
{
}

void Occc::on_recv(const MsgPack &msg)
{
	if (nullptr != m_owner_channel.m_shm_queue.get())
	{
		LOG_ERROR("no shm");
		return;
	}
	m_owner_channel.m_shm_queue->Send(msg.data, msg.len);
}

void Occc::on_connected()
{
	m_owner_channel.OnConnected();	
}

void Occc::on_disconnected()
{
	if (!ChannelMgr::Instance().FreeChannel(m_owner_channel.GetId()))
	{
		LOG_ERROR("FreeChannel fail. id=%d", m_owner_channel.GetId());
	}
}

OuterChannelServer::OuterChannelServer()
:m_ocsc_id(0)
{
}

bool OuterChannelServer::Init(uint64 oscs_id, uint64 mc_id, const sockaddr_in &svr_addr)
{
	m_ocsc_id = oscs_id;
	if (!BaseChannel::Init(mc_id, svr_addr))
	{
		return false;
	}
	MgrConnector *mc = GetMc();
	if (nullptr == mc)
	{
		LOG_ERROR("find mc fail");
		return false;
	}
	shmem &sw = GetShmPsW();
	shmem &sr = GetShmPsR();

	ShmQueueMgr *sqm = new ShmQueueMgr();
	if (!sqm->Init(sr.get_key(), sw.get_key(), sr.get_size()))
	{
		LOG_ERROR("open shm fail. ");
		delete sqm;
		return false;
	}
	m_shm_queue.reset(sqm);

	//通知 ps 地址反过来给对对方进程读写
	ntf_create_ps_channel res;
	res.svr_addr = GetSvrAddr();
	GetShareShmForPS(res.shm);
	mc->Send(res);
	LOG_DEBUG("Ocsc create shm, notify pS");
	return true;
}


Ocsc *OuterChannelServer::FindOcsc()
{
	ChannelServer *svr = ChannelServerMgr::Instance().FindSvr(GetSvrAddr());
	if (nullptr == svr)
	{
		LOG_ERROR("find svr fail, port=%d", ntohs(GetSvrAddr().sin_port));
		return nullptr;
	}

	return svr->FindOcsc(m_ocsc_id);
}

void OuterChannelServer::TrySendData()
{
	if (nullptr == m_shm_queue.get())
	{
		LOG_ERROR("OuterChannelServer no bus io");
		return;
	}
	if (0 == m_shm_queue->reader_msg_count())
	{
		return;
	}
	MsgPack msg;
	size_t out_len = 0;
	if (!m_shm_queue->Recv(msg.data, wyl::ArrayLen(msg.data), out_len))
	{
		return;
	}
	msg.len = (unsigned short)out_len;

	Ocsc *ocsc = FindOcsc();
	if (nullptr == ocsc)
	{
		LOG_ERROR("find Ocsc  fail, port=%d, id=%llu", ntohs(GetSvrAddr().sin_port), m_ocsc_id);
		return;
	}
	ocsc->send_data(msg);
}

void OuterChannelServer::OnDestoryChannel()
{
	MgrConnector *mc = BusMgr::Instance().FindMc(GetMcId());
	if (nullptr == mc)
	{
		LOG_ERROR("find mc fail, id=%llu", GetMcId());
		return;
	}
	if (mc->IsConnect())
	{
		ntf_disconnect_ps ntf;
		ntf.ps_write_key = GetShmPsW().get_key();
		mc->Send(ntf);
	}

	//try free ocsc
	Ocsc *ocsc = FindOcsc();
	if (nullptr == ocsc)
	{
		LOG_DEBUG("try find Ocsc  fail, port=%d, id=%llu.", ntohs(GetSvrAddr().sin_port), m_ocsc_id);
		return;
	}
	if (ocsc->FreeSelf())
	{
		LOG_DEBUG("ocsc free ok");
	}
	else
	{
		LOG_DEBUG("ignore ocsc free, maybe call by ocsc disconnect");
	}
}

InnerChannel::InnerChannel()
:m_ps_mc_id(0)
, m_pc_mc_id(0)
{

}

bool InnerChannel::Init(uint64 ps_mc_id, uint64 pc_mc_id, const sockaddr_in &svr_addr)
{
	m_pc_mc_id = pc_mc_id;
	m_ps_mc_id = ps_mc_id;
	return  BaseChannel::Init(ps_mc_id, svr_addr);
}

void InnerChannel::OnDestoryChannel()
{
	if (MgrConnector * pc_mc = BusMgr::Instance().FindMc(m_pc_mc_id))
	{
		if (pc_mc->IsConnect())
		{
			ntf_disconnect_pc ntf;
			ntf.svr_addr = GetSvrAddr();
			pc_mc->Send(ntf);
		}
	}
	else
	{
		LOG_ERROR("find pc_mc fail. mc_id=%llu", m_pc_mc_id);
	}

	if (MgrConnector * ps_mc = BusMgr::Instance().FindMc(m_ps_mc_id))
	{
		if (ps_mc->IsConnect())
		{
			ntf_disconnect_ps ntf;
			ntf.ps_write_key = GetShmPsW().get_key();
			ps_mc->Send(ntf);
		}
	}
	else
	{
		LOG_ERROR("find ps_mc fail. mc_id=%llu", m_ps_mc_id);
	}

}
