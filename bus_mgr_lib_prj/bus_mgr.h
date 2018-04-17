/*
author: yiliang.wu

功能：busmgr 管理进程和进程之间的连接, mgr 和 p必须同一台机器

*/

#pragma once
#include "include_all.h"
#include <map>
#include "channel.h"
#include "utility/misc.h"
#include <memory.h>
#include "bus_typedef.h"

 
//mc = mgr connector, p 通过socket连接mgr, 在mgr进程的connector
class MgrConnector : public ListenerConnector
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
	virtual void OnRecv(const MsgPack &msg) override;
	virtual void OnConnected() override;
	virtual void onDisconnected();

private:
};


//检查outer channel 读写定时器
class CheckOuterChannelTimer : public BaseLeTimer
{
private:
	virtual void OnTimer(void *user_data) override;
};

//bus mgr总管理器,一切从这里出发
class BusMgr
{
public:
	static BusMgr &Instance()
	{
		static BusMgr d;
		return d;
	}
	bool Init(unsigned short listen_port);
	void Run();
	MgrConnector *FindMc(uint64 mc_id);
private:
	Listener<MgrConnector> m_listener;
	CheckOuterChannelTimer m_timer;
};
