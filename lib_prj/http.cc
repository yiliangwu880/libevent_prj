
#include "http.h"
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>  
#include "include_all.h"
#include "utility/config.h" //Ŀǰ�����ã���ֲɾ��
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
	//�󶨵�ָ����ַ��
	int ret = evhttp_bind_socket(m_evhttp, ip, port);
	if (ret != 0) {
		LOG_FATAL("init http fail");
		return false;
	}
	evhttp_set_gencb(m_evhttp, RevRequestCB, this);
	return true;
}

void BaseHttpSvr::Reply(int code, const char *reason, const std::string str/*=""*/)
{
	if (nullptr == m_tmp_req)
	{
		LOG_FATAL("error call, repeated call ReplyError or Reply");//ReplyError ���� Reply �������λ���
		return;
	}
	//����Ҫʹ�õ�buffer����
	evbuffer* buf = evbuffer_new();
	evbuffer_add_printf(buf, "%s", str.c_str());
	evhttp_send_reply(m_tmp_req, code, reason, buf);//�ͷ���Դ
	evbuffer_free(buf);

	m_tmp_req = nullptr;
}

void BaseHttpSvr::ReplyError(int code, const char *reason)
{
	if (nullptr == m_tmp_req)
	{
		LOG_FATAL("error call, repeated call ReplyError or Reply");//ReplyError ���� Reply �������λ���
		return;
	}
	evhttp_send_error(m_tmp_req, code, reason); //�ͷ���Դ
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
	p->RevRequest();//���������� evhttp_send_error ���� evhttp_send_reply �ͷ���Դ
	if (nullptr != p->m_tmp_req) //�û�û�����ͷ���Դ����
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
		LOG_ERROR("nullptr == p->m_tmp_req");//������RevRequest���棬��Ӧǰ���ã�
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

	//����url
	////////////////////////////////////
	typedef std::unique_ptr<evhttp_uri, decltype(&::evhttp_uri_free)> HttpUriPtr;
	HttpUriPtr hu(evhttp_uri_parse(url), ::evhttp_uri_free); //�Զ��ͷ���Դ
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


	// ��ʼ��evdns_base_new
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
	// �������Ӷ���ɹ���, ���ùرջص�����
	evhttp_connection_set_closecb(m_connection, connection_close_callback, this);
	// ����evhttp_request�������÷���״̬��Ӧ�Ļص�����
	struct evhttp_request* req = evhttp_request_new(remote_read_callback, this);//request�����Լ�evhttp_request_free, ����m_connection����
	// ���httpͷ��
	struct evkeyvalq* header = evhttp_request_get_output_headers(req);
	evhttp_add_header(header, "Host", host);
	evhttp_add_header(header, "Content-type", "application/json");

	if (nullptr != post_data)
	{
		evbuffer_add(req->output_buffer, post_data, strlen(post_data));
	}
	// ����http����
	evhttp_make_request(m_connection, req, cmd_type, uri); //���ú�m_connection����req���ͷ�
	evhttp_connection_set_timeout(m_connection, ot_sec);  //���ó�ʱ
	//LOG_DEBUG("request ok");
	return true;
}


void BaseHttpClient::remote_read_callback(struct evhttp_request* remote_rsp, void* arg)
{
	BaseHttpClient *p = (BaseHttpClient *)arg;
	if (remote_rsp)
	{
		int start_num = remote_rsp->response_code / 100;
		if (start_num != 4 && start_num !=5) //��400,500��ͷ��
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
	{	//��ʱû��Ӧ����������ʧ��
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
	//delete p; ���ﲻ��Ҫ�� Ŀǰ�������н������̶������ remote_read_callback���������ͷžͿ����ˡ�
}
