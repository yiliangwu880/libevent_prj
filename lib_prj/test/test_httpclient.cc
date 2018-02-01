
#include "../include_all.h"
#include "../utility/config.h" //Ŀǰ�����ã���ֲɾ��
#include "../utility/BacktraceInfo.hpp"
#include "../http.h"
#include "../utility/misc.h"

using namespace std;
unsigned int get_proc_mem()
{

	const int VMRSS_LINE = 17;
	unsigned int pid = getpid();
	char file_name[64] = { 0 };
	FILE *fd;
	char line_buff[512] = { 0 };
	sprintf(file_name, "/proc/%d/status", pid);

	fd = fopen(file_name, "r");
	if (nullptr == fd){
		return 0;
	}

	char name[64];
	int vmrss;
	for (int i = 0; i < VMRSS_LINE - 1; i++){
		fgets(line_buff, sizeof(line_buff), fd);
	}

	fgets(line_buff, sizeof(line_buff), fd);
	sscanf(line_buff, "%s %d", name, &vmrss);
	fclose(fd);

	return vmrss;
}

namespace
{
	const uint16 server_port = 33433;

	class ClientMgr;
	class MyClient : public BaseHttpClient
	{
	public:
		MyClient()
		{}
		~MyClient()
		{
			//LOG_DEBUG("free");
		}
	private:
		virtual void Reply(const char *str) override;
	};


	void MyClient::Reply(const char *str)
	{
		//LOG_DEBUG("replay:");
		//LOG_DEBUG("%s", str);

	}
	// ��ȡ����ռ���ڴ�

	class MyTimer : public BaseLeTimer
	{
	private:
		virtual void OnTimer(void *user_data) override
		{
			MyClient *p = MyClient::Create<MyClient>();
			//p->Request("http://192.168.141.196:33433/abc", EVHTTP_REQ_GET, 3);
			p->Request("http://www.baidu.com", EVHTTP_REQ_GET,5);
			//p->Request("http://com", EVHTTP_REQ_GET, 5);

			static int cnt = 0;
			if (cnt%1000 == 0)
			{
				LOG_DEBUG("memory=%d", get_proc_mem());
			}
			cnt++;
		}
	};

	void test1()
	{
		LibEventMgr::Instance().Init();

		
		MyTimer o;
		o.StartTimer(100, 0, true);
		
		LibEventMgr::Instance().dispatch();

		LOG_DEBUG("========================end=====================");
	}

} //end namespace

void test_httpclient()
{
	LOG_DEBUG("start test_httpclient");
	test1();
}