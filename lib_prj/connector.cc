
#include "lev_mgr.h"
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>  
#include "utility/config.h" //目前调试用，移植删掉
#include "connector.h"
#include "include_all.h"

using namespace std;


BaseConnect::BaseConnect()
:m_buf_e(nullptr)
, m_fd(0)
, m_msbs(1024 * 64)
, m_is_connect(false)
, m_msg_write_len(0)
, m_no_ev_cb_log(false)
{
	memset(&m_addr, 0, sizeof(m_addr));
}

BaseConnect::~BaseConnect()
{
	free();
}

void BaseConnect::free()
{
	if (m_buf_e)
	{
		bufferevent_free(m_buf_e);
		//LOG_DEBUG("bufferevent_free(m_buf_e)");
		m_buf_e = 0;
	}
	if (m_fd != 0)
	{
		
		//释放m_buf_e，的时候，库里面会释放m_fd
		//if (0 != ::close(m_fd))
		//{
		//	LOG_ERROR("::close fail , fd=%d", m_fd);
		//}
		//LOG_DEBUG("::close(m_fd)");
		m_fd = 0;
	}
	m_is_connect = false;
}


bool BaseConnect::BaseAcceptInit(evutil_socket_t fd, struct sockaddr* sa)
{
	if (0 != m_fd)
	{
		LOG_ERROR("repeated init");
		return false;
	}
	m_fd = fd;
	//LOG_DEBUG("AcceptInit FD=%d", m_fd);
	m_buf_e = bufferevent_socket_new(LibEventMgr::Instance().GetEventBase(), fd, BEV_OPT_CLOSE_ON_FREE); //释放m_buf_e，的时候，库里面会释放m_fd
	if (!m_buf_e)
	{
		LOG_ERROR("cannot bufferevent_socket_new libevent ...\n");
		return false;
	}
	
	memcpy(&m_addr, (sockaddr_in*)sa, sizeof(m_addr));

	bufferevent_setcb(m_buf_e, readcb, nullptr, eventcb, this);
	bufferevent_enable(m_buf_e, EV_WRITE | EV_READ);

	m_is_connect = true;
	on_connected();
	return true;
}

bool BaseConnect::ConnectByAddr()
{
	if (0 == m_addr.sin_port)
	{
		LOG_ERROR("m_addr don't init");
		return false;
	}
	m_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_fd < 0)
	{
		LOG_ERROR("m_fd < 0");
		return false;
	}
	m_buf_e = bufferevent_socket_new(LibEventMgr::Instance().GetEventBase(), m_fd, BEV_OPT_CLOSE_ON_FREE);//提示你提供给bufferevent_socket_new() 的套接字务必是非阻塞模式, 为此LibEvent 提供了便利的方法	evutil_make_socket_nonblocking.
	if (nullptr == m_buf_e)
	{
		LOG_ERROR("nullptr == m_buf_e");
		if (0 != ::close(m_fd))
		{
			LOG_ERROR("::close fail , fd=%d", m_fd);
		}
		m_fd = 0;
		return false;
	}
	bufferevent_setcb(m_buf_e, readcb, nullptr, eventcb, this);
	bufferevent_enable(m_buf_e, EV_WRITE | EV_READ);

	if (bufferevent_socket_connect(m_buf_e, (struct sockaddr*)&m_addr, sizeof(m_addr)) == 0)//连接失败会里面关闭fd
	{
		return true;
	}
	LOG_ERROR("bufferevent_socket_connect fail"); //这里没跑过，不知道什么情况才跑
	m_fd = 0;
	return false;
}

bool BaseConnect::ConnectInit(const char* connect_ip, unsigned short connect_port)
{
	if (0 != m_fd)
	{
		LOG_ERROR("repeated init");
		return false;
	}
	if (nullptr == connect_ip)
	{
		LOG_ERROR("nullptr == connect_ip");
		return false;
	}

	memset(&m_addr, 0, sizeof(m_addr));
	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.s_addr = inet_addr(connect_ip);
	m_addr.sin_port = htons(connect_port);
	return ConnectByAddr();
}


bool BaseConnect::ConnectInit(const sockaddr_in &svr_addr)
{
	if (0 != m_fd)
	{
		LOG_ERROR("repeated init");
		return false;
	}

	memset(&m_addr, 0, sizeof(m_addr));
	m_addr = svr_addr;
	return ConnectByAddr();
}

bool BaseConnect::TryReconnect()
{
	if (!IsConnect())
	{
		return ConnectByAddr();
	}
	else
	{
		return true; //不需要重连
	}
}

void BaseConnect::DisConnect()
{
	free();
	on_disconnected();
}

void BaseConnect::writecb(struct bufferevent* bev, void* user_data)
{
	((BaseConnect*)user_data)->conn_write_callback(bev);
}

void BaseConnect::eventcb(struct bufferevent* bev, short events, void* user_data)
{
	((BaseConnect*)user_data)->conn_event_callback(bev, events);
}

void BaseConnect::readcb(struct bufferevent* bev, void* user_data)
{
	((BaseConnect*)user_data)->conn_read_callback(bev);
}

void BaseConnect::conn_write_callback(bufferevent* bev)
{
	//如果不用 考虑删掉，
}

bool BaseConnect::IsWaitCompleteMsg() const
{
	return 0 != m_msg_write_len;
}

void BaseConnect::conn_read_callback(bufferevent* bev)
{
	const static int HEAD_LEN = sizeof(MsgPack::len);
	int input_len = evbuffer_get_length(bufferevent_get_input(bev));

	while (input_len > 0)
	{
		char *write_addr = ((char *)&m_msg) + m_msg_write_len;
		//根据2种状态去做不同读取操作
		//状态1, msg.len没读完整,等待读完整过程
		if (m_msg_write_len < HEAD_LEN)
		{
			int need_to_read = HEAD_LEN - m_msg_write_len;
			int write_len = min(input_len, need_to_read);
			int ret_write_len = bufferevent_read(bev, write_addr, write_len);
			if (ret_write_len != write_len)
			{
				LOG_FATAL("ret_write_len != write_len");
				DisConnect();
				return;
			}
			input_len -= ret_write_len;
			m_msg_write_len += ret_write_len;
			continue;
		}
		//状态2, msg.len完整，等待读取完整消息
		else 
		{
			if (m_msg.len > MAX_MSG_DATA_LEN) //包过大，断开连接
			{
				LOG_ERROR("rev msg len too big. %d", m_msg.len);
				DisConnect();
				return;
			}

			int need_to_read = m_msg.len + HEAD_LEN - m_msg_write_len;
			if (need_to_read < 0)
			{
				LOG_FATAL("need_to_read < 0");
				DisConnect();
				return;
			}
			int write_len = min(input_len, need_to_read);
			int ret_write_len = bufferevent_read(bev, write_addr, write_len);
			if (ret_write_len != write_len)
			{
				LOG_FATAL("ret_write_len != write_len");
				DisConnect();
				return;
			}
			input_len -= ret_write_len;
			m_msg_write_len += ret_write_len;

			if (need_to_read == ret_write_len)// 接收完整
			{	
				on_recv(m_msg);
				//重置m_msg,等下次接收新消息
				m_msg.len = 0;
				m_msg_write_len = 0;
			}
			continue;
		}
	}
}


void BaseConnect::conn_event_callback(bufferevent* bev, short events)
{
	if (events & BEV_EVENT_CONNECTED)
	{
		m_is_connect = true;
		on_connected();
	}
	else
	{
		if (false == m_no_ev_cb_log)
		{
			if (events & BEV_EVENT_EOF)
			{
				LOG_DEBUG("event cb:connection closed.\n");
			}
			else if (events & BEV_EVENT_ERROR)
			{
				LOG_DEBUG("event cb:got an error on the connection: %s\n", strerror(errno));
			}
			else if (events & BEV_EVENT_READING)
			{

			}
			else if (events & BEV_EVENT_WRITING)
			{

			}
			else if (events & BEV_EVENT_TIMEOUT)
			{

			}
		}
		on_error(events); 
		DisConnect();
		return; //这里本对象可能已经销毁，别再引用
	}
}

bool BaseConnect::send_data(const MsgPack &msg)
{
	if (!m_is_connect)
	{
		LOG_ERROR("is disconnect");
		return false;
	}
	const char* data = (const char*)&msg;
	int len = msg.len + sizeof(msg.len);
	if (0 == m_fd)
	{
		LOG_ERROR("BaseConnect not init. 0 == m_fd");
		return false; 
	}
	if (!m_buf_e)
	{
		LOG_ERROR("BaseConnect not init !m_buf_e");
		return false;
	}

	if (m_msbs != 0)
	{
		struct evbuffer* output = bufferevent_get_output(m_buf_e);
		if (!output)
		{
			LOG_FATAL("unknow error");
			return false;
		}
		size_t output_len = evbuffer_get_length(output);
		if (output_len > m_msbs)
		{
			LOG_ERROR("too much bytes wait to send %ld", output_len);
			DisConnect();
			return false;
		}
	}

	if (0 != bufferevent_write(m_buf_e, data, len))
	{
		LOG_ERROR("bufferevent_write fail, len=%d", len);
		return false;
	}
	return true;
}

bool BaseConnect::send_data_no_head(const char* data, int len)
{
	if (!m_is_connect)
	{
		LOG_ERROR("is disconnect");
		return false;
	}
	if (0 == m_fd)
	{
		LOG_ERROR("BaseConnect not init. 0 == m_fd");
		return false;
	}
	if (!m_buf_e)
	{
		LOG_ERROR("BaseConnect not init !m_buf_e");
		return false;
	}
	if (nullptr == data)
	{
		LOG_ERROR("nullptr == data\n");
		return false;
	}

	if (m_msbs != 0)
	{
		struct evbuffer* output = bufferevent_get_output(m_buf_e);
		if (!output)
		{
			LOG_FATAL("unknow error");
			return false;
		}
		size_t output_len = evbuffer_get_length(output);
		if (output_len > m_msbs)
		{
			LOG_ERROR("too much bytes for send %ld", output_len);
			DisConnect();
			return false;
		}
	}

	if (0 != bufferevent_write(m_buf_e, data, len))
	{
		LOG_ERROR("bufferevent_write fail, len=%d", len);
		return false;
	}
	return true;
}
void BaseConnect::setwatermark(short events, unsigned int lowmark, unsigned int highmark)
{
	if (0 == m_fd)
	{
		LOG_ERROR("BaseConnect not init. 0 == m_fd");
		return;
	}
	if (!m_buf_e)
	{
		LOG_ERROR("BaseConnect not init !m_buf_e");
		return;
	}
	bufferevent_setwatermark(m_buf_e, events, lowmark, highmark);
}

void BaseConnect::GetRemoteAddr(std::string &ip, unsigned short &port) const
{
	ip = inet_ntoa(m_addr.sin_addr);
	port = ntohs(m_addr.sin_port);
}


