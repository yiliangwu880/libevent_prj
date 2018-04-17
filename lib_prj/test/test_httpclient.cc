
#include "../include_all.h"
#include "../utility/config.h" //目前调试用，移植删掉
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
	// 获取进程占用内存

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


	class EtcdClient : public BaseHttpClient
	{
	private:
		virtual void Reply(const char *str) override
		{//json格式字符串
			LOG_DEBUG("rev:%s", str);
			
		}
		virtual void ReplyError(int err_code, const char *err_str) override
		{
			LOG_DEBUG("rev error %d:%s", err_code, err_str);
		}
	};
} //end namespace

void test_httpclient()
{
	LOG_DEBUG("start test_httpclient");
	test1();
}
void test_etcd()
{
	LOG_DEBUG("start test_etcd");
	LibEventMgr::Instance().Init();

	EtcdClient *p = MyClient::Create<EtcdClient>();
	////为etcd存储的键赋值 
	////$ curl -X PUT http://127.0.0.1:2379/v2/keys/message -d value="Hello"
	//const char *data = "value=\"Hello\"";
	//p->Request("http://127.0.0.1:2379/v2/keys/message", EVHTTP_REQ_PUT, 5, data);	 //这样不行
	//p->Request("http://127.0.0.1:2379/v2/keys/message?value=Hello", EVHTTP_REQ_PUT, 5);



	////查询etcd某个键存储的值  curl http://127.0.0.1:2379/v2/keys/message
	//p->Request("http://127.0.0.1:2379/v2/keys/message", EVHTTP_REQ_GET);
	////删除一个值
	//p->Request("http://127.0.0.1:2379/v2/keys/message", EVHTTP_REQ_DELETE);
	////自动在目录下创建有序键
	//p->Request("http://127.0.0.1:2379/v2/keys/queue?value=Job1", EVHTTP_REQ_POST);

	//Watching the directory  curl http://127.0.0.1:2379/v2/keys/message?wait=true\&recursive=true
	//p->Request("http://127.0.0.1:2379/v2/keys/message?wait=true&recursive=true", EVHTTP_REQ_GET, 3600*24);

	//Test and set  可以做锁  curl -X PUT http://127.0.0.1:2379/v2/keys/message?prevValue=Hello -d value=Hi
	p->Request("http://127.0.0.1:2379/v2/keys/message?prevValue=123&value=Hi", EVHTTP_REQ_PUT, 5);

	LibEventMgr::Instance().dispatch();

	LOG_DEBUG("========================end=====================");
}
