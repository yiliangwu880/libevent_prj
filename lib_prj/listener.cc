
#include "lev_mgr.h"
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include "include_all.h"


using namespace std;


BaseConnectorMgr::~BaseConnectorMgr()
{
	for (const auto &v : m_all_connector)
	{
		delete v.second;
	}
	m_all_connector.clear();
}

bool BaseConnectorMgr::CloseConnect(uint64 id)
{
	auto it = m_all_connector.find(id);
	if (it == m_all_connector.end())
	{
		LOG_FATAL("free connector can't find. id=%llu", id);
		return false;
	}
	ListenerConnector *p = it->second;
	m_all_connector.erase(it);
	delete p;
	return true;
}

ListenerConnector * BaseConnectorMgr::FindConnect(uint64 id)
{
	auto it = m_all_connector.find(id);
	if (it == m_all_connector.end())
	{
		return nullptr;
	}
	return it->second;
}


ListenerConnector * BaseConnectorMgr::CreateConnectForListener()
{
	ListenerConnector *p = NewConnect();
	if (nullptr == p)
	{
		return p;
	}
	m_all_connector[p->GetId()] = p;
	return p;
}


ListenerConnector::ListenerConnector()
:m_cn_mgr(nullptr)
, m_id(0)
, m_ignore_free(false)
{
	static uint64 id_seed = 0;//�����ظ����Ӿͻ��ظ���ʵ���ϲ��������ô����
	m_id = ++id_seed;
	memset(&m_svr_addr, 0, sizeof(m_svr_addr));
}

ListenerConnector::~ListenerConnector()
{
	m_cn_mgr = nullptr; //���������ָ�룬�Ŵ���������Ұָ�롣
}

bool ListenerConnector::AcceptInit(evutil_socket_t fd, struct sockaddr* sa, const sockaddr_in &svr_addr)
{
	m_svr_addr = svr_addr;
	return BaseConnect::BaseAcceptInit(fd, sa);
}

bool ListenerConnector::FreeSelf()
{
	if (m_ignore_free)
	{
		return false;
	}
	return m_cn_mgr->CloseConnect(m_id); 
}

void ListenerConnector::on_disconnected()
{
	m_ignore_free = true;
	onDisconnected();
	m_ignore_free = false;
	FreeSelf();
	//LOG_DEBUG("ListenerConnector::on_disconnected");
}
