
#include "http.h"
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>  
#include "include_all.h"
#include "utility/logFile.h" //目前调试用，移植删掉
#include <memory>

using namespace std;

BaseHttpSvr::BaseHttpSvr()
:m_evhttp(nullptr)
, m_tmp_req(nullptr)
{
}

BaseHttpSvr::~BaseHttpSvr()
{
	if (nullptr != m_evhttp)
	{
		evhttp_free(m_evhttp);
	}
}

bool BaseHttpSvr::Init(unsigned short port/*=80*/, const char* ip /*= nullptr*/)
{
	if (nullptr != m_evhttp)
	{
		LOG_FATAL("repeated init");
		return false;
	}
	m_evhttp = evhttp_new(LibEventMgr::Instance().GetEventBase());
	if (nullptr == m_evhttp) {
		LOG_FATAL("init http fail");
		return false;
	}
	if (nullptr == ip)
	{
		ip = "0.0.0.0";
	}
	//绑定到指定地址上
	int ret = evhttp_bind_socket(m_evhttp, ip, port);
	if (ret != 0) {
		LOG_FATAL("init http fail");
		return false;
	}
	//evhttp_set_gencb(m_evhttp, RevRequestCB, this);
	evhttp_set_cb(m_evhttp, "/phoneop/", RevRequestCB, this);
	return true;
}

void BaseHttpSvr::Reply(int code, const char *reason, const std::string str/*=""*/)
{
	if (nullptr == m_tmp_req)
	{
		LOG_FATAL("error call, repeated call ReplyError or Reply");//ReplyError 或者 Reply 调用两次或者
		return;
	}
	//创建要使用的buffer对象
	evbuffer* buf = evbuffer_new();
	evbuffer_add_printf(buf, "%s", str.c_str());
	evhttp_send_reply(m_tmp_req, code, reason, buf);//释放资源
	evbuffer_free(buf);

	m_tmp_req = nullptr;
}

void BaseHttpSvr::ReplyError(int code, const char *reason)
{
	if (nullptr == m_tmp_req)
	{
		LOG_FATAL("error call, repeated call ReplyError or Reply");//ReplyError 或者 Reply 调用两次或者
		return;
	}
	evhttp_send_error(m_tmp_req, code, reason); //释放资源
	m_tmp_req = nullptr;
}

void BaseHttpSvr::RevRequestCB(struct evhttp_request* req, void* arg)
{
	BaseHttpSvr *p = (BaseHttpSvr *)arg;
	if (nullptr != p->m_tmp_req)
	{
		LOG_FATAL("nullptr != p->m_tmp_req");
	}
	p->m_tmp_req = req;
	p->RevRequest();//里面必须调用 evhttp_send_error 或者 evhttp_send_reply 释放资源
	if (nullptr != p->m_tmp_req) //用户没调用释放资源函数
	{
		evhttp_send_error(p->m_tmp_req, HTTP_SERVUNAVAIL, "svr have error code");
		p->m_tmp_req = nullptr;
		LOG_FATAL("miss call ReplyError or Reply in RevRequest function");
	}
	//evhttp_request_free(req);
}

const char *BaseHttpSvr::GetUri()
{
	if (nullptr == m_tmp_req)
	{
		LOG_ERROR("nullptr == p->m_tmp_req");//必须在RevRequest里面，响应前调用，
		return nullptr; 
	}
	const char *uri = evhttp_request_get_uri(m_tmp_req);
	return uri;
}


BaseHttpClient::BaseHttpClient()
:m_dnsbase(nullptr)
, m_connection(nullptr)
, m_is_request(false)
{

}

BaseHttpClient::~BaseHttpClient()
{
	if (nullptr != m_dnsbase)
	{
		evdns_base_free(m_dnsbase, true);
	}

	if (nullptr != m_connection)
	{
		evhttp_connection_free(m_connection);
	}
}

bool BaseHttpClient::Request(const char *url, evhttp_cmd_type cmd_type, unsigned int ot_sec, const char *post_data)
{
	if (m_is_request)
	{
		ReplyError(500, "repeated Request");
		delete this;
		return false;
	}
	if (nullptr == url)
	{
		ReplyError(500, "nullptr == url");
		delete this;
		return false;
	}

	//解析url
	////////////////////////////////////
	typedef std::unique_ptr<evhttp_uri, decltype(&::evhttp_uri_free)> HttpUriPtr;
	HttpUriPtr hu(evhttp_uri_parse(url), ::evhttp_uri_free); //自动释放资源
	if (nullptr == hu.get())
	{
		ReplyError(500, "Parse url failed! ");
		delete this;
		return false;
	}

	const char* host = evhttp_uri_get_host(hu.get());
	if (!host)
	{
		ReplyError(500, "Parse host failed! ");
		delete this;
		return false;
	}

	int port = evhttp_uri_get_port(hu.get());
	if (port < 0) port = 80;

	const char* uri = url;
	const char* path = evhttp_uri_get_path(hu.get());
	if (path == NULL || strlen(path) == 0)
	{
		uri = "/";
	}
	/////////////////////////////////////////


	// 初始化evdns_base_new
	m_dnsbase = evdns_base_new(LibEventMgr::Instance().GetEventBase(), 1);
	if (!m_dnsbase)
	{
		ReplyError(500, "Create dns base failed!");
		delete this;
		return false;
	}

	m_connection = evhttp_connection_base_new(LibEventMgr::Instance().GetEventBase(), m_dnsbase, host, port);
	if (!m_connection)
	{
		ReplyError(500, "Create evhttp connection failed!");
		delete this;
		return false;
	}
	// 创建连接对象成功后, 设置关闭回调函数
	evhttp_connection_set_closecb(m_connection, connection_close_callback, this);
	// 创建evhttp_request对象，设置返回状态响应的回调函数
	struct evhttp_request* req = evhttp_request_new(remote_read_callback, this);//request不用自己evhttp_request_free, 交给m_connection管理
	// 添加http头部
	struct evkeyvalq* header = evhttp_request_get_output_headers(req);
	evhttp_add_header(header, "Host", host);
	evhttp_add_header(header, "Content-type", "application/json");

	if (nullptr != post_data)
	{
		evbuffer_add(req->output_buffer, post_data, strlen(post_data));
	}
	// 发起http请求
	evhttp_make_request(m_connection, req, cmd_type, uri); //调用后，m_connection管理req的释放
	evhttp_connection_set_timeout(m_connection, ot_sec);  //设置超时
	//LOG_DEBUG("request ok");
	return true;
}


void BaseHttpClient::remote_read_callback(struct evhttp_request* remote_rsp, void* arg)
{
	BaseHttpClient *p = (BaseHttpClient *)arg;
	if (remote_rsp)
	{
		int start_num = remote_rsp->response_code / 100;
		if (start_num != 4 && start_num !=5) //非400,500开头的
		{
			//LOG_DEBUG("remote_read_callback Code: %d %s", remote_rsp->response_code, remote_rsp->response_code_line);
			//LOG_DEBUG("replay:");
			int input_len = evbuffer_get_length(remote_rsp->input_buffer);
			static const int MAX_LEN = 1024 * 100;
			char write_addr[MAX_LEN];
			int ret_write_len = evbuffer_remove(remote_rsp->input_buffer, write_addr, MAX_LEN);
			if (input_len != ret_write_len)
			{
				LOG_ERROR("read data incomplete %d %d", input_len, ret_write_len);
			}
			
			//LOG_DEBUG("%s", write_addr);
			p->Reply(write_addr);
		}
		else
		{
			p->ReplyError(remote_rsp->response_code, remote_rsp->response_code_line);
			//LOG_ERROR("remote_read_callback Code: %d %s", remote_rsp->response_code, remote_rsp->response_code_line);
		}
	}
	else
	{	//超时没响应，或者连接失败
		p->ConnectFail();
		//LOG_ERROR("remote_read_callback remote respond prt == NULL");
	}
	
	// evhttp_connection_free(m_conn_ptr);
	//p->m_connection = nullptr;
	//LOG_DEBUG("HttpClient Respond Free connection");
	//LOG_DEBUG("remote_read_callback free BaseHttpClient");
	delete p;
}

void BaseHttpClient::connection_close_callback(struct evhttp_connection* connection, void* arg)
{
	BaseHttpClient *p = (BaseHttpClient *)arg;
	if (p->m_connection != connection)
	{
		LOG_ERROR("p->m_connection != connection %llu %llu", p->m_connection, connection);
	}
	p->m_connection = nullptr;
	//LOG_DEBUG("connection_close_callback");
	//delete p; 这里不需要， 目前发现所有结束流程都会调用 remote_read_callback，再哪里释放就可以了。
}
