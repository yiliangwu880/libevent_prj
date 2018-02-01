
#include "bus_mgr.h"
#include "utility/BacktraceInfo.hpp"
#include <string>
#include <ifaddrs.h>
#include <signal.h>

using namespace std;
namespace
{
	const unsigned short bus_mgr_port = 45553;

	class MyTimer : public BaseLeTimer
	{
	public:
		virtual void OnTimer(void *p){};
	};

	void catch_SIGINT(int sig)
	{
		exit(1);
	}
	void StartMgr()
	{
		LOG_DEBUG("-------------StartMgr-------------");
		if (!BusMgr::Instance().Init(bus_mgr_port))
		{
			LOG_ERROR("init fail");
			return;
		}
		ChannelMgr::Instance().Init(2000); //¿ÉÑ¡
		LibEventMgr::Instance().dispatch();
	}

	void CreateShm()
	{
		LibEventMgr::Instance().Init();
		{
			shmem sm;
			//if (!sm.Create(0x2000, 1000))
			//{
			//	LOG_DEBUG("create fail");
			//	shmem r;
			//	r.attach(0x2000, 1000);
			//	r.remove();
			//}
			//else
			//{
			//	LOG_DEBUG("create ok");
			//}
			sm.Create(0x2000, 1000);
			sm.detach();
			sm.remove();


			//LOG_DEBUG("size=%d", sm.get_size());
		}
		{
			//shmem sm2;
			//sm2.open_and_attach(0x2001, 1000);
			//sm2.remove();
		}
		MyTimer t;
		t.StartTimer(1000*11, 0);
		LibEventMgr::Instance().dispatch();
	}


}


void test_bus_mgr_base()
{
	//LOG_DEBUG("-------------start test_bus_mgr_base-------------");
	CBacktraceInfo::Instance().RegHangUpHandler();
	signal(SIGINT, catch_SIGINT);

	//CreateShm();
	StartMgr();
	

	LOG_DEBUG("-------------end-------------");
}
