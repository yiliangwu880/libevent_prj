/*


BaseHttpClient使用例子：
class MyClient : public BaseHttpClient
{
private:
	virtual void Reply(struct evhttp_request* remote_rsp) override
	{
	}
};

*/

#pragma once

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/dns.h>
#include <event2/http_struct.h>
#include <string>

/*
常用libevent api参考
const char *uri = evhttp_request_get_uri(req);
struct evhttp_uri *decoded = evhttp_uri_parse(uri);
evhttp_send_error(req, HTTP_BADREQUEST, 0);
int32_t iCmd  = evhttp_request_get_command(req);
const char *query_part = evhttp_uri_get_query(decoded);
*/
class BaseHttpSvr
{
public:
	BaseHttpSvr();
	virtual ~BaseHttpSvr();
	bool Init(unsigned short port=80, const char* ip = nullptr);

	//@param code the HTTP response code to send,  参考 HTTP_OK 宏定义
	//@param reason a brief message to send with the response code
	void Reply(int code, const char *reason, const std::string str = "");

	void ReplyError(int code, const char *reason);
	const char *GetUri();
private:
	virtual void RevRequest() = 0;
	static void RevRequestCB(struct evhttp_request* req, void* arg);

private:
	evhttp *m_evhttp;

	evhttp_request *m_tmp_req; //nullptr表示evhttp_request对象已经释放

};


class BaseHttpClient
{
public:
	BaseHttpClient();
	virtual ~BaseHttpClient();

	bool Request(const char *url, evhttp_cmd_type cmd_type = EVHTTP_REQ_GET, unsigned int ot_sec = 30, const char *post_data = nullptr);

	//分配一个对象使用，连接结束会自动释放。用户代码不需要写释放语句
	//另外Request失败也会释放对象.
	//建议写法： BaseHttpClient::Create<MyClient>().Request(..);
	//或者:
	//{
	// MyClient *p =  BaseHttpClient::Create<MyClient>();
	// p->Init(); p->Request();
	//}//目的不让p漏出去导致野指针
	template<class MyClient>
	static MyClient *Create()
	{
		MyClient *p = new MyClient();
		return p;
	}
private:
	virtual void Reply(const char *str) = 0;
	//服务器响应错误信息
	virtual void ReplyError(int err_code, const char *err_str){}; 
	//超时没响应，或者连接失败
	virtual void ConnectFail(){};
	//请求连接失败，超时接收不到消息响应回调
	//情况：
	//服务器连不通
	static void remote_read_callback(struct evhttp_request* remote_rsp, void* arg);
	//待测试, 调用情况：
	//超时(服务器接收连接，但不超时响应的 情况先调用这个，后调用remote_read_callback)
	static void connection_close_callback(struct evhttp_connection* connection, void* arg);

private:
	struct evdns_base* m_dnsbase;
	struct evhttp_connection* m_connection;
	bool m_is_request;
};
