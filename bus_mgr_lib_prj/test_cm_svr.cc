

#include "bus_client_mgr.h"
#include "utility/BacktraceInfo.hpp"
namespace
{
	const unsigned short bus_mgr_port = 45553;
	
	//const char *svr_ip1 = "192.168.141.196"; //会认为是外网地址
	const char *svr_ip1 = "127.0.0.1";
	const unsigned short svr_port1 = 15550;

	class SvrConnector : public BusServerConnector
	{
	public:
	private:
		//每次接收都是完整消息包
		virtual void on_recv(const char* buf, size_t len)
		{
			LOG_DEBUG("SvrConnector rev msg:%s, and echo msg", buf);
			Send(buf, len);
		}
		virtual void on_connected()
		{
			LOG_DEBUG("SvrConnector on_connected");
		}
	};
	class SvrConnector2 : public BusServerConnector
	{
	public:
	private:
		//每次接收都是完整消息包
		virtual void on_recv(const char* buf, size_t len)
		{
			LOG_DEBUG("SvrConnector2 rev msg");
		}
		virtual void on_connected()
		{
			LOG_DEBUG("SvrConnector2 on_connected");
		}
	};

	void TestSvr()
	{
		LOG_DEBUG("-------------TestSvr-------------");
		LibEventMgr::Instance().Init();

		ClientBusMgr::Instance().Init(bus_mgr_port);

		BusServerT<SvrConnector> svr;
		svr.Init(svr_port1, svr_ip1);


		//BusServerT<SvrConnector2> svr2;
		//svr2.Init(svr_port1, svr_ip1);

		LibEventMgr::Instance().dispatch();
	}
}

void test_mgr_svr_base()
{
	CBacktraceInfo::Instance().RegHangUpHandler();
	TestSvr();

}
