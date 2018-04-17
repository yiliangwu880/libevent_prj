
#include "bus_client_mgr.h"
#include "channel_server.h"
#include "utility/BacktraceInfo.hpp"

namespace
{
	const unsigned short bus_mgr_port = 45553;
	//const char *svr_ip1 = "192.168.141.196"; //会认为是外网地址
	const char *svr_ip1 = "127.0.0.1";
	const unsigned short svr_port1 = 15550;
	class MyBusClientConnector : public BusClientConnector
	{
	public:

	private:
		virtual void OnRecv(const char* buf, size_t len) override
		{
			LOG_DEBUG("OnConnected OnRecv: %s", buf);
		}
		virtual void OnConnected()  override
		{
			LOG_DEBUG("OnConnected, send abc ");
			//PrintShmMsg();
			Send("abc", 4);
			PrintShmMsg();
		}
	};

	void test_client()
	{
		LOG_DEBUG("-------------test_client-------------");
		LibEventMgr::Instance().Init();

		ClientBusMgr::Instance().Init(bus_mgr_port);

		MyBusClientConnector c;
		c.ConnectInit(svr_port1, svr_ip1);

		LibEventMgr::Instance().dispatch();
	}
}

//测试inner channel,用本地ip比如192.168.141.196

void test_mgr_client_base()
{
	CBacktraceInfo::Instance().RegHangUpHandler();
	test_client();
}
