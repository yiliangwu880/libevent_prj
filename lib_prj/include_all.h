/*
只合适单线程用。
使用实际例子看 test_server.cc test_client.cc
使用例子：
客户端：
	#include "../include_all.h"

	class MyConnectClient1 : public BaseConnect
	{
	private:
		virtual void OnRecv(const MsgPack &msg) override
		{

		}
		virtual void OnConnected() override
		{
			MsgPack msg;
			...初始化msg
			send_data(msg);
		}
		virtual void on_disconnected() override
		{
		}
	};
	void main()
	{
		LibEventMgr::Instance().Init();

		MyConnectClient1 connect1;
		connect1.ConnectInit(ip, server_port);

		LibEventMgr::Instance().dispatch();
	}

服务器:

	class Connect2Client : public ListenerConnector
	{
	public:

	private:
		virtual void OnRecv(const MsgPack &msg) override
		{
			LOG_DEBUG("OnRecv %s", &msg.data);
			send_data(msg);
			if (0)
			{//some time do this destory connect
				FreeSelf();
			}
		}
		virtual void OnConnected() override
		{
			LOG_DEBUG("OnConnected");
		}
		virtual void onDisconnected() override {}
	};

	//简单模型
	void main()
	{
		LibEventMgr::Instance().Init();

		Listener<Connect2Client> listener;
		listener.Init(server_port);

		LibEventMgr::Instance().dispatch();
	}

	//指定connector对象分配器模式
	void main()
	{
		class ConnectorMgr : public BaseConnectorMgr
		{
		private:
		virtual ListenerConnector *CreateConnect(){...};

		};
		LibEventMgr::Instance().Init();
		ConnectorMgr mgr;
		Listener<> listener(mgr);
		listener.Init(server_port);

		LibEventMgr::Instance().dispatch();
	}	
*/

#pragma once

#include "lev_mgr.h"
#include "listener.h"
#include "connector.h"
#include "timer_mgr.h"

