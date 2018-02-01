#include "bus_mgr.h"
#include "utility/config.h" //目前调试用，移植删掉
#include "utility/BacktraceInfo.hpp"
#include "bus_proto.h"
#include "channel.h"
#include "channel_server.h"

using namespace Proto;
using namespace wyl;
using namespace std;

bool BusMgr::Init(unsigned short listen_port)
{
	LibEventMgr::Instance().Init();

	return m_listener.Init(listen_port, "127.0.0.1");
}

void BusMgr::Run()
{
	m_timer.StartTimer(10, nullptr, true);
	LibEventMgr::Instance().dispatch();
}


MgrConnector * BusMgr::FindMc(uint64 mc_id)
{
	MgrConnector *mc = (MgrConnector *)m_listener.m_cn_mgr.FindConnect(mc_id);
	return mc;
}



void MgrConnector::on_recv(const MsgPack &msg_pack)
{
	const BaseCmd *msg = (const BaseCmd*)(msg_pack.data);
	switch (msg->cmd_id)
	{
	default:
		LOG_ERROR("unknow cmd_id=%d", msg->cmd_id);
		break;
	case req_reg_svr_cmd:
		{
			const req_reg_svr *req = (const req_reg_svr *)msg;
			ChannelServerMgr::Instance().McRevReqRegSvr(*this, req);
		}
		break;
	case req_connect_svr_cmd:
		{
			const req_connect_svr *req = (const req_connect_svr *)msg;
			ChannelMgr::Instance().McRevReqConnectSvr(GetId(), req->svr_addr);
		}
		break;
	case pm_ntf_disconnect_ps_cmd:
		{
			const pm_ntf_disconnect_ps *req = (const pm_ntf_disconnect_ps *)msg;
			ChannelMgr::Instance().FreeChannel(req->ps_write_key);
		}
		break;
	case pm_ntf_disconnect_pc_cmd:
		{
			const pm_ntf_disconnect_pc *req = (const pm_ntf_disconnect_pc *)msg;
			ChannelMgr::Instance().FreeChannel(req->ps_write_key);
		}
		break;
	}
}

void MgrConnector::on_connected()
{

}

void MgrConnector::onDisconnected()
{
	ChannelMgr::Instance().OnMcDisconnect(GetId());
	ChannelServerMgr::Instance().OnMcDisconnect(GetId());
}

namespace
{
	void MyForeachChannel(BaseChannel *channel)
	{
		if (nullptr == channel)
		{
			LOG_ERROR("nullptr == channel");
			return;
		}
		channel->TrySendData();
	}
}
void CheckOuterChannelTimer::OnTimer(void *user_data)
{
	ChannelMgr::Instance().ForeachAll(MyForeachChannel);
}
