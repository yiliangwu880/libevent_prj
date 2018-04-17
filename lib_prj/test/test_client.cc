
#include "../include_all.h"
#include "../utility/config.h" //目前调试用，移植删掉
#include "../utility/BacktraceInfo.hpp"
#include <unistd.h>

using namespace std;
unsigned int get_proc_mem();
namespace
{
	const uint16 server_port = 33433;
	const char *ip = "127.0.0.1";
	// 获取进程占用内存

	void Str2MsgPack(const string str, MsgPack &msg)
	{
		msg.len = str.length()+1;
		memcpy(msg.data, str.c_str(), str.length()+1);
	}

	class MyTimer : public BaseLeTimer 
	{
	private:
		virtual void OnTimer(void *user_data) override;
	};



	class MyConnectClient1 : public BaseConnect
	{
	private:
		virtual void OnRecv(const MsgPack &msg) override
		{
			LOG_DEBUG("1 OnRecv %s", &msg.data);
		
		}
		virtual void OnConnected() override
		{
			LOG_DEBUG("1 OnConnected, send first msg");
			MsgPack msg;
			Str2MsgPack("1 msg", msg);
			send_data(msg);
		}
		virtual void OnError(short events) override
		{
			LOG_DEBUG("on_error");
		}
		virtual void on_disconnected() override
		{

		}
	};
	class MyConnectClient2 : public BaseConnect
	{
	private:
		virtual void OnRecv(const MsgPack &msg) override
		{
			LOG_DEBUG("2 OnRecv %s", &msg.data);


		}
		virtual void OnConnected() override
		{
			LOG_DEBUG("2 OnConnected, send first msg");
			MsgPack msg;
			Str2MsgPack("2 msg", msg);
			send_data(msg);
		}
		virtual void OnError(short events) override
		{
			LOG_DEBUG("on_error");
		}
		virtual void on_disconnected() override
		{

		}
	};
	void TestTruncation(MyConnectClient1 *p)
	{
		MsgPack msg;
		Str2MsgPack("123", msg);
		//截断发出去
		{
			char *pData = (char *)&msg;
			static int offset = 0;
			static int state = 0;
			LOG_DEBUG("state %d", state);
			switch (state)
			{
			default:
				LOG_ERROR("error state");
				break;
			case 0: //长度发一半
				offset = 0;
				p->send_data_no_head(pData + offset, 1);
				offset += 1;
				state = 1;
				break;
			case 1://长度完整
				p->send_data_no_head(pData + offset, 1);
				offset += 1;
				state = 2;
				break;
			case 2://内容发一半
				p->send_data_no_head(pData + offset, 2);
				offset += 2;
				state = 3;
				break;
			case 3://内容完整+下一个长度一半
				p->send_data_no_head(pData + offset, 2);
				p->send_data_no_head(pData, 1);

				offset = 1;
				state = 1;
				break;
			}
		}


		p->TryReconnect();
	}
	void TestSvrDisconnect(MyConnectClient1 *p)
	{
		MsgPack msg;
		Str2MsgPack("del", msg);
		p->send_data(msg);
		LOG_DEBUG("send del");
	}
	void MyTimer::OnTimer(void *user_data)
	{
		LOG_DEBUG("on time");
		MyConnectClient1 *p = (MyConnectClient1 *)user_data;
		//TestTruncation(p);
		TestSvrDisconnect(p);
		StopTimer();
	}
	void nouse()
	{
		if (ip)
		{
		}
		if (server_port)
		{
		}
	}

	class NewDelConnector : public BaseConnect
	{
	private:
		virtual void OnRecv(const MsgPack &msg) override
		{

		}
		virtual void OnConnected() override
		{
			MsgPack msg;
			Str2MsgPack("1 msg", msg);
			send_data(msg);
		}
		virtual void OnError(short events) override
		{
			LOG_DEBUG("on_error");
		}
		virtual void on_disconnected() override
		{
			LOG_DEBUG("on_disconnected");
		}
	};
	class TestCloseConnectTimer : public BaseLeTimer
	{
		enum State
		{
			START_S,
			CONNECT_S,
			DISCONNECT_S,
		};
	public:
		TestCloseConnectTimer()
		{
			m_s = START_S;
			cnt = 0;
		}

		virtual void OnTimer(void *user_data) override
		{
			switch (m_s)
			{
			default:
				StopTimer();
				break;
			case START_S:
				m_client = new NewDelConnector();
				m_client->ConnectInit(ip, server_port);
				m_client->SetEventCbLog(true);
				m_s = CONNECT_S;
				//LOG_DEBUG("new client");
				break;
			case CONNECT_S:
				delete m_client;
				m_client = nullptr;
				cnt++;
				//LOG_DEBUG("free client");
				m_s = START_S;
				break;

			}
		}
		BaseConnect *m_client;
		State m_s;
		int cnt;
	};



	class MyConnectClientFree : public BaseConnect
	{
	private:
		virtual void OnRecv(const MsgPack &msg) override
		{
			//LOG_DEBUG("1 OnRecv %s", &msg.data);

		}
		virtual void OnConnected() override
		{
		//	LOG_DEBUG("1 OnConnected, send first msg");
			MsgPack msg;
			Str2MsgPack("free msg", msg);
			send_data(msg);
		}
		virtual void OnError(short events) override
		{
			LOG_DEBUG("on_error %d", events);
		}
		virtual void on_disconnected() override
		{

		}
	};
	class ShowMemoryTimer : public BaseLeTimer
	{
	public:
		ShowMemoryTimer()
		{
			cnt = 0;
		}
		int cnt;
	private:
		virtual void OnTimer(void *user_data) override
		{
			LOG_DEBUG("memory = %d", get_proc_mem());
		}
	};
	class TestFreeTimer : public BaseLeTimer
	{
	public:
		TestFreeTimer()
		{
			cnt = 0;
		}
		int cnt;
		MyConnectClientFree *m_client;
	private:
		virtual void OnTimer(void *user_data) override
		{
			if (0 == cnt%2)
			{
				m_client = new MyConnectClientFree();
				m_client->ConnectInit(ip, server_port);
				
				//new int[10];
			}
			else
			{
				delete m_client;
				m_client = nullptr;
			}
			cnt++;
		}
	};
	void testFree()
	{
		CBacktraceInfo::Instance().RegHangUpHandler();
		LibEventMgr::Instance().Init();
		TestFreeTimer tt;
		tt.StartTimer(1, nullptr, true);
		ShowMemoryTimer st;
		st.StartTimer(1000 * 60 * 5, nullptr, true);

		LibEventMgr::Instance().dispatch();

	}

	void test1()
	{
		CBacktraceInfo::Instance().RegHangUpHandler();
		LibEventMgr::Instance().Init();

		MyConnectClient1 connect1;
		connect1.ConnectInit(ip, server_port);
		
		MyConnectClient2 connect2;
		connect2.ConnectInit(ip, server_port);

		//MyTimer o;
		//o.StartTimer(1000, &connect1, true);
		TestCloseConnectTimer t;
		t.StartTimer(1, nullptr, true);
		ShowMemoryTimer st;
		st.StartTimer(1000 * 60 * 5, nullptr, true);


		LibEventMgr::Instance().dispatch();
	}



	class MyConnectClientFail : public BaseConnect
	{
	private:
		virtual void OnRecv(const MsgPack &msg) override
		{
			LOG_DEBUG("1 OnRecv %s", &msg.data);

		}
		virtual void OnConnected() override
		{
			LOG_DEBUG("1 OnConnected, send first msg");
		}
		virtual void OnError(short events) override
		{
			LOG_DEBUG("on_error events=%d", events);
		}
		virtual void on_disconnected() override
		{

		}
	};


	class EmptyTimer : public BaseLeTimer
	{
	public:
		EmptyTimer()
		{
			cnt = 0;
		}
		int cnt;
	private:
		virtual void OnTimer(void *user_data) override
		{
		}
	};
	void testConnectTimeOut()
	{
		CBacktraceInfo::Instance().RegHangUpHandler();
		LibEventMgr::Instance().Init();

		MyConnectClientFail c;
		c.ConnectInit(ip, server_port-10); //这里有BUG，连接失败，会退出进程
		EmptyTimer t;
		t.StartTimer(10000, 0, true);
		LibEventMgr::Instance().dispatch();
		LOG_DEBUG("-----------end--------");
	}


	void testTime()
	{

	}
} //end namespace

void test_client()
{
	LOG_DEBUG("start test_client");
	//test1(); 
	testFree();
	//testConnectTimeOut();
}