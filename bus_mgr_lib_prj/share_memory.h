/*
author: yiliang.wu

功能：管理共享内存
使用例子：

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
#include "utility/logFile.h" //目前调试用，移植删掉
#include <memory>

class shmem
{
public:
	shmem();
	~shmem();

	bool attach(int32_t key, int32_t size);
	//创建共享内存，如果key已经被创建，就会返回失败
	bool Create(int32_t key, int32_t size);
	bool detach();
	//删除共享内存，如果还有进程引用，就先删shm标识，以后不能再attach. 等没有进程引用了，会自动彻底删除.
	//attach 进程退出时，系统会自动detach.
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

//无锁队列控制器
struct BusSimplexQueueCtrl
{
	volatile size_t wr_ptr; 
	volatile size_t rd_ptr;
	volatile size_t wr_count; //写消息数
	volatile size_t rd_count; //读消息数
};

//管理一个无锁环形队列
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
	@brief					: 数据出队列缓冲区，增加out_len参数，
	@param buf				: 数据缓冲区
	@param in_len			: 数据缓冲区大小
	@param out_len			: 真实获取的数据缓冲区大小, out_len <= in_len
	@param peek				: true表示只是看看，不真正读取
	@return					: 0，成功；-1，失败
	*/
	bool Read(char* buf, size_t in_len, size_t& out_len, bool peek = false);

	size_t MsgCount() const;

private:
	bool _Write(const char* buf, size_t len, int flags = 0);

	BusSimplexQueueCtrl &m_ctrl_;
	char* m_base_;
	size_t m_size_;//共享内存空间大小

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
	size_t m_AllocSize = 0; //m_vecBufferHold 分配的总字节数	
};

//共享内存读写管理
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

	//para buf 接收地址
	//para in_len buf 可写长度
	//para out_len 写入长度
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
