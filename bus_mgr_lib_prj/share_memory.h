/*
author: yiliang.wu

���ܣ��������ڴ�
ʹ�����ӣ�

main()
{
	ShareMemory obj = memory_mgr::Open(key);
	use obj.addr
	
}

*/

#pragma once
#include <stdint.h>
#include <deque>
#include <memory.h>
#include "utility/logFile.h" //Ŀǰ�����ã���ֲɾ��
#include <memory>

class shmem
{
public:
	shmem();
	~shmem();

	bool attach(int32_t key, int32_t size);
	//���������ڴ棬���key�Ѿ����������ͻ᷵��ʧ��
	bool Create(int32_t key, int32_t size);
	bool detach();
	//ɾ�������ڴ棬������н������ã�����ɾshm��ʶ���Ժ�����attach. ��û�н��������ˣ����Զ�����ɾ��.
	//attach �����˳�ʱ��ϵͳ���Զ�detach.
	bool remove();

	void* get_addr();
	int get_size() const;
	int get_key() const { return m_key; }

private:
	bool open_and_attach(int32_t key, int32_t size);

private:
	void*	addr_;
	int		shmid;
	int m_key;
};

struct BusMsgHead
{
	unsigned int len;
};

//�������п�����
struct BusSimplexQueueCtrl
{
	volatile size_t wr_ptr; 
	volatile size_t rd_ptr;
	volatile size_t wr_count; //д��Ϣ��
	volatile size_t rd_count; //����Ϣ��
};

//����һ���������ζ���
class BusSimplexQueue
{
public:
	BusSimplexQueue();
	BusSimplexQueue(char* base, size_t size);
	~BusSimplexQueue() { }
	//void Init(char* base, size_t size);
	bool Write(const char* buf, size_t len, int flags = 0);
	void Flush();
	size_t GetHoldSize() const { return m_AllocSize; }

	/**
	@brief					: ���ݳ����л�����������out_len������
	@param buf				: ���ݻ�����
	@param in_len			: ���ݻ�������С
	@param out_len			: ��ʵ��ȡ�����ݻ�������С, out_len <= in_len
	@param peek				: true��ʾֻ�ǿ�������������ȡ
	@return					: 0���ɹ���-1��ʧ��
	*/
	bool Read(char* buf, size_t in_len, size_t& out_len, bool peek = false);

	size_t MsgCount() const;

private:
	bool _Write(const char* buf, size_t len, int flags = 0);

	BusSimplexQueueCtrl &m_ctrl_;
	char* m_base_;
	size_t m_size_;//�����ڴ�ռ��С

	struct BufferHold
	{

		void AllocBuffer(const char* _buf, size_t _len)
		{
			m_buf = new char[_len];
			m_len = _len;
			memcpy(m_buf, _buf, _len);
		}

		void RelaseBuffer()
		{
			delete m_buf;
			m_buf = nullptr;
			m_len = 0;
		}


		char* m_buf = nullptr;
		size_t m_len = 0;
	};

	std::deque<BufferHold> m_vecBufferHold;
	size_t m_AllocSize = 0; //m_vecBufferHold ��������ֽ���	
};

//�����ڴ��д����
class ShmQueueMgr
{
public:
	ShmQueueMgr();
	~ShmQueueMgr();
	bool Init(int write_key, int read_key, unsigned int len);

	size_t reader_msg_count()
	{
		return m_r_queue->MsgCount();
	}

	size_t writer_msg_count()
	{
		return m_w_queue->MsgCount();
	}


	bool Send(const char* buf, size_t len)
	{
		return m_w_queue->Write(buf, len);
	}

	void Flush()
	{
		m_w_queue->Flush();
	}

	size_t GetHoldSize()
	{
		return m_w_queue->GetHoldSize();
	}

	//para buf ���յ�ַ
	//para in_len buf ��д����
	//para out_len д�볤��
	bool Recv(char* buf, size_t in_len, size_t& out_len)
	{
		return m_r_queue->Read(buf, in_len, out_len);
	}

private:
	shmem m_w_shm;
	shmem m_r_shm;
	std::unique_ptr<BusSimplexQueue> m_w_queue; //write memory
	std::unique_ptr<BusSimplexQueue> m_r_queue; //read memory
};
