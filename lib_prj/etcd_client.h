/*
访问etcd 的客户端
*/
#pragma once

#include "http.h"
#include "utility/logFile.h" //移植删掉



class BaseEtcdClient
{
	class HttpClient : public BaseHttpClient
	{
		BaseEtcdClient *m_owner;
	public:
		void Init(BaseEtcdClient *owner)
		{
			m_owner = owner;
		}

	private:
		virtual void Reply(const char *str) override
		{//json格式字符串
			if (nullptr == m_owner)
			{
				return;
			}
			m_owner->Reply(str);

		}
		virtual void ReplyError(int err_code, const char *err_str) override
		{
			if (nullptr == m_owner)
			{
				return;
			}
			m_owner->ReplyError(err_code, err_str);
		}
	};
public:
	BaseEtcdClient()
	{
		m_pre_url = "http://127.0.0.1:2379/v2/keys/";
	}
	void Init(const std::string &dns_name = "127.0.0.1:2379")
	{
		m_pre_url = "http://" + dns_name + "/v2/keys/";
	}
	//设置键值，
	// para key, 格式: message 或者 message/d1/dir
	void Set(const std::string &key, const std::string &value)
	{
		HttpClient *p = BaseHttpClient::Create<HttpClient>();
		p->Init(this);
		std::string s = m_pre_url + key + "?value=" + value;
		p->Request(s.c_str(), EVHTTP_REQ_PUT);
	}
	void Get(const std::string &key)
	{
		HttpClient *p = BaseHttpClient::Create<HttpClient>();
		p->Init(this);
		std::string s = m_pre_url + key;
		p->Request(s.c_str(), EVHTTP_REQ_GET);
	}
	void Del(const std::string &key)
	{
		HttpClient *p = BaseHttpClient::Create<HttpClient>();
		p->Init(this);
		std::string s = m_pre_url + key;
		p->Request(s.c_str(), EVHTTP_REQ_DELETE);
	}
	//自动在目录下创建有序键
	void CreateQueue(const std::string &dir_key, const std::string &value = "none")
	{
		HttpClient *p = BaseHttpClient::Create<HttpClient>();
		p->Init(this);
		std::string s = m_pre_url + dir_key + "?value=" + value;
		p->Request(s.c_str(), EVHTTP_REQ_POST);
	}
	//侦听键的值被修改
	void Watch(const std::string &key)
	{
		HttpClient *p = BaseHttpClient::Create<HttpClient>();
		p->Init(this);
		std::string s = m_pre_url + key + "?wait=true&recursive=true";
		p->Request(s.c_str(), EVHTTP_REQ_GET);
	}
	//条件设置  可以做锁 ， 当前值==prevValue时，才会成功
	void ConditionSet(const std::string &key, const std::string &prevValue, const std::string &value)
	{
		HttpClient *p = BaseHttpClient::Create<HttpClient>();
		p->Init(this);
		std::string s = m_pre_url + key + "?prevValue=" + prevValue + "&value=" + value;
		p->Request(s.c_str(), EVHTTP_REQ_PUT);
	}
private:
	//para str json格式字符串
	virtual void Reply(const char *str) 
	{
		LOG_DEBUG("rev:%s", str);
	}
	virtual void ReplyError(int err_code, const char *err_str)
	{
		LOG_DEBUG("rev error %d:%s", err_code, err_str);
	}
private:
	std::string m_pre_url; //url 前缀http://127.0.0.1:2379/v2/keys
};