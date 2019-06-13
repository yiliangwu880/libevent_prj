
#include "../include_all.h"
#include "../utility/logFile.h" //目前调试用，移植删掉
#include "../utility/BacktraceInfo.hpp"
#include "../utility/single_progress.hpp"
#include <unistd.h>
#include <signal.h>

using namespace std;
unsigned int get_proc_mem();
namespace
{
	void Str2MsgPack(const string str, MsgPack &msg)
	{
		msg.len = str.length() + 1;
		memcpy(msg.data, str.c_str(), str.length() + 1);
	}
	const uint16 server_port = 32430;

	static evutil_socket_t s_fd = 0;
	class MyTimer : public BaseLeTimer
	{
	private:
		virtual void OnTimer(void *user_data) override
		{
			evutil_socket_t fd = *(evutil_socket_t *)user_data;
			if (0 != ::close(fd))
					{
				LOG_ERROR("::close fail , fd=%d", fd);
			}
		LOG_DEBUG("timer ::close(m_fd) %d", s_fd);
		}
	};

	MyTimer s_t;
	class Connect2Client : public ListenerConnector
	{
	public:

	private:
		virtual void OnRecv(const MsgPack &msg) override
		{
			LOG_DEBUG("OnRecv %s", &msg.data);
			send_data(msg);
			if (msg.data == string("del"))
			{//some time do this destory connect
				LOG_DEBUG("req del connect");
				FreeSelf();
			}

		}
		virtual void OnConnected() override
		{
			m_com.SetEventCbLog(true);
			LOG_DEBUG("server OnConnected");
			MsgPack msg;
			Str2MsgPack("s", msg);
			send_data(msg);
		}
		virtual void onDisconnected() override {}
	};

	class ShowMemoryTimer : public BaseLeTimer
	{
	public:
		ShowMemoryTimer()
		{
			cnt = 0;
		}
	private:
		virtual void OnTimer(void *user_data) override
		{
			LOG_DEBUG("memory = %d", get_proc_mem());
			cnt++;
		}
		int cnt;
	};

	void test1()
	{
		LOG_DEBUG("test1");
		CBacktraceInfo::Instance().RegHangUpHandler();
		LibEventMgr::Instance().Init();

		Listener<Connect2Client> listener;
		listener.Init(server_port);
		ShowMemoryTimer t;
		t.StartTimer(1000*60*5, 0, true);
		LibEventMgr::Instance().dispatch();

		LOG_DEBUG("========================end=====================");
	}

	class Connect2ClientFree : public ListenerConnector
	{
	public:

	private:
		virtual void OnRecv(const MsgPack &msg) override
		{
		//	LOG_DEBUG("OnRecv %s", &msg.data);
			send_data(msg);
	

		}
		virtual void OnConnected() override
		{
			m_com.SetEventCbLog(true);
		//	LOG_DEBUG("server OnConnected");
			MsgPack msg;
			Str2MsgPack("s", msg);
			send_data(msg);
		}
		virtual void onDisconnected() override {}
	};
	void testFree()
	{
		CBacktraceInfo::Instance().RegHangUpHandler();
		LibEventMgr::Instance().Init();

		Listener<Connect2ClientFree> listener;
		listener.Init(server_port);
		ShowMemoryTimer t;
		t.StartTimer(1000, 0, true);
		LibEventMgr::Instance().dispatch();

		LOG_DEBUG("========================end=====================");
	}

	const  char * app_name = "test_name";

	class StopProgressTimer : public BaseLeTimer
	{
	public:
	private:
		virtual void OnTimer(void *user_data) override
		{
			if (SingleProgress::Instance().IsExit())
			{
				LOG_DEBUG("timer exit progress");
				exit(1);
			}
		}
	};
	void TestFileLock_start()
	{
		SingleProgress::Instance().CheckSingle(app_name);

		LibEventMgr::Instance().Init();
		StopProgressTimer t;
		t.StartTimer(10, 0, true);
		LibEventMgr::Instance().dispatch();
	}
	void TestFileLock_stop()
	{
		SingleProgress::Instance().StopSingle(app_name);
	}

	void UserSignalCB(int sig_type)
	{
		LOG_DEBUG("run UserSignalCB");

	}


	void testSignalReg()
	{
		CBacktraceInfo::Instance().RegHangUpHandler();
		LibEventMgr::Instance().Init();
		LibEventMgr::Instance().RegSignal(SIGUSR2, UserSignalCB);
		Listener<Connect2ClientFree> listener;
		listener.Init(server_port);
		ShowMemoryTimer t;
		t.StartTimer(1000, 0, true);
		LibEventMgr::Instance().dispatch();

		LOG_DEBUG("========================end=====================");
	}
} //end namespace

void test_server()
{
	LOG_DEBUG("start test_server");
	test1();
	//testFree();
	//testSignalReg();
}

void TestFileLock()
{
	TestFileLock_start(); 
	//TestFileLock_stop();
}