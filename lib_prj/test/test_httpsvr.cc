
#include "../include_all.h"
#include "../utility/config.h" //目前调试用，移植删掉
#include "../utility/BacktraceInfo.hpp"
#include "../http.h"

using namespace std;

namespace
{
	const uint16 server_port = 33433;

	class MyHttpSvr : public BaseHttpSvr
	{
	public:

	private:
		virtual void RevRequest() override
		{
			//const char *uri = GetUri(req);
			//LOG_DEBUG("on_recv req, uri=%s", uri);

			//Reply(HTTP_OK, "ok", "reply 123");
			ReplyError(HTTP_BADMETHOD, "abc");
		}
	};

	void test1()
	{
		LibEventMgr::Instance().Init();

		MyHttpSvr svr;
		svr.Init(server_port);

		LibEventMgr::Instance().dispatch();

		LOG_DEBUG("========================end=====================");
	}

} //end namespace

void test_httpsvr()
{
	LOG_DEBUG("start test_httpsvr");
	test1();
}