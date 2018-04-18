/*
����etcd �Ŀͻ���
*/
#pragma once

#include "http.h"
#include "utility/config.h" //��ֲɾ��



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
		{//json��ʽ�ַ���
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
	//���ü�ֵ��
	// para key, ��ʽ: message ���� message/d1/dir
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
	//�Զ���Ŀ¼�´��������
	void CreateQueue(const std::string &dir_key, const std::string &value = "none")
	{
		HttpClient *p = BaseHttpClient::Create<HttpClient>();
		p->Init(this);
		std::string s = m_pre_url + dir_key + "?value=" + value;
		p->Request(s.c_str(), EVHTTP_REQ_POST);
	}
	//��������ֵ���޸�
	void Watch(const std::string &key)
	{
		HttpClient *p = BaseHttpClient::Create<HttpClient>();
		p->Init(this);
		std::string s = m_pre_url + key + "?wait=true&recursive=true";
		p->Request(s.c_str(), EVHTTP_REQ_GET);
	}
	//��������  �������� �� ��ǰֵ==prevValueʱ���Ż�ɹ�
	void ConditionSet(const std::string &key, const std::string &prevValue, const std::string &value)
	{
		HttpClient *p = BaseHttpClient::Create<HttpClient>();
		p->Init(this);
		std::string s = m_pre_url + key + "?prevValue=" + prevValue + "&value=" + value;
		p->Request(s.c_str(), EVHTTP_REQ_PUT);
	}
private:
	//para str json��ʽ�ַ���
	virtual void Reply(const char *str) 
	{
		LOG_DEBUG("rev:%s", str);
	}
	virtual void ReplyError(int err_code, const char *err_str)
	{
		LOG_DEBUG("rev error %d:%s", err_code, err_str);
	}
private:
	std::string m_pre_url; //url ǰ׺http://127.0.0.1:2379/v2/keys
};