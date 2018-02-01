#include <sys/types.h>
#include <unistd.h>
#include <sys/shm.h>
#include <stdio.h>
#include <limits>

#include "share_memory.h"

shmem::shmem()
{
	addr_ = nullptr;
	shmid = 0;
	m_key = 0;
}


shmem::~shmem()
{
	detach();
}

bool shmem::attach(int32_t key, int32_t size)
{
	m_key = key;
	shmid = shmget(key, size, 0666);
	if (shmid < 0)
	{
		LOG_ERROR("shmget");
		return false;
	}

	if ((addr_ = (char *)shmat(shmid, NULL, 0)) == (char *)-1)
	{
		LOG_ERROR("shmat");
		addr_ = nullptr;
		return false;
	}
	return true;
}

bool shmem::Create(int32_t key, int32_t size)
{
	if (0 != shmid)
	{
		LOG_ERROR("shmem have init");
		return false;
	}
	//check can open?
	int id = shmget(key, size, 0666);
	if (-1 != id)
	{
		//LOG_ERROR("shmget have been create. key=%d", key);
		return false;
	}

	return open_and_attach(key, size);
}

bool shmem::open_and_attach(int32_t key, int32_t size)
{
	m_key = key;
	shmid = shmget(key, size, 0666 | IPC_CREAT);
	if (shmid < 0)
	{
		LOG_ERROR("shmget key=%d", key);
		return false;
	}
	if ((addr_ = (char *)shmat(shmid, NULL, 0)) == (char *)-1)
	{
		addr_ = nullptr;
		LOG_ERROR("shmat, key=%d", key);
		return false;
	}
	return true;
}

bool shmem::remove()
{
	return 0 == shmctl(shmid, IPC_RMID, nullptr);
}


void* shmem::get_addr()
{
	if (nullptr == addr_)
	{
		LOG_ERROR("nullptr == addr_");
	}
	return addr_;
}

bool shmem::detach()
{
	if (nullptr == addr_)
	{
		return false;
	}
	if (0 != shmdt(addr_)) //shm�Ѿ����� ɾ��������������ʧ�ܡ������������
	{
		LOG_DEBUG("call shmdt fail, %p", addr_);
		return false;
	}
	return true;
}

int shmem::get_size() const
{
	shmid_ds d;
	if (0 != shmctl(shmid, IPC_STAT, &d))
	{
		LOG_ERROR("get_size fail");
		return 0;
	}
	return d.shm_segsz;
}

BusSimplexQueue::BusSimplexQueue(char* base, size_t size)
: m_ctrl_(*(BusSimplexQueueCtrl*)(base))
, m_base_(base + sizeof(BusSimplexQueueCtrl))
, m_size_(size - sizeof(BusSimplexQueueCtrl))
{
}


static const size_t MAX_HOLD_SIZE = 100 * 1024 * 1024;
bool BusSimplexQueue::Write(const char* buf, size_t len, int flags)
{
	if (buf == NULL || len == 0)
	{
		LOG_ERROR("error para. buf == NULL || len == 0");
		return false;
	}

	while (true)
	{
		// wr_ptr��д���ݵ�ָ��
		size_t wr_ptr = m_ctrl_.wr_ptr;
		// rd_ptr�Ƕ����ݵ�ָ��
		size_t rd_ptr = m_ctrl_.rd_ptr;

		// space��ʣ��ռ��С, size_�ǹ����ڴ�ռ��С
		size_t space = wr_ptr >= rd_ptr ? (m_size_ - wr_ptr + rd_ptr + 1) : (rd_ptr - wr_ptr - 1);
		//���������������Ϣ, �Ƚ��������ڵ���Ϣ���빲���ڴ�ͨ��
		if (m_vecBufferHold.empty() == false)
		{
			auto& refData = m_vecBufferHold.front();
			if (space >= refData.m_len + sizeof (BusMsgHead))
			{
				_Write(refData.m_buf, refData.m_len, flags);
				m_AllocSize -= refData.m_len;
				refData.RelaseBuffer();
				m_vecBufferHold.pop_front();
				continue;
			}
			else
			{
				if (m_AllocSize >= MAX_HOLD_SIZE)
				{
					LOG_ERROR("buffer memory too big.");
					return false;
				}
				BufferHold hold;
				hold.AllocBuffer(buf, len);
				m_AllocSize += len;
				m_vecBufferHold.emplace_back(std::move(hold));
				return true;
			}

		}

		// ����ռ䲻��, ֱ�ӷ���, ����������Ͼ��Ǳ�������
		if (space < len + sizeof (BusMsgHead))
		{
			//����ڲ���������û��,���ȷ����ڲ�������
			if (m_AllocSize < MAX_HOLD_SIZE)
			{
				BufferHold hold;
				hold.AllocBuffer(buf, len);
				m_AllocSize += len;
				m_vecBufferHold.emplace_back(std::move(hold));
				return true;
			}


			return false;
		}

		return _Write(buf, len, flags);
	}//while (true)
}


void BusSimplexQueue::Flush()
{
	while (true)
	{
		// wr_ptr��д���ݵ�ָ��
		size_t wr_ptr = m_ctrl_.wr_ptr;
		// rd_ptr�Ƕ����ݵ�ָ��
		size_t rd_ptr = m_ctrl_.rd_ptr;

		// space��ʣ��ռ��С, size_�ǹ����ڴ�ռ��С
		size_t space = wr_ptr >= rd_ptr ? (m_size_ - wr_ptr + rd_ptr + 1) : (rd_ptr - wr_ptr - 1);
		//���������������Ϣ, �Ƚ��������ڵ���Ϣ���빲���ڴ�ͨ��
		if (m_vecBufferHold.empty() == false)
		{
			auto& refData = m_vecBufferHold.front();
			if (space >= refData.m_len + sizeof (BusMsgHead))
			{
				_Write(refData.m_buf, refData.m_len, 0);
				m_AllocSize -= refData.m_len;
				refData.RelaseBuffer();
				m_vecBufferHold.pop_front();
				continue;
			}
		}

		return;
	}
}

bool BusSimplexQueue::_Write(const char* buf, size_t len, int flags)
{
	if (buf == NULL || len == 0)
	{
		LOG_ERROR("error para, buf == NULL || len == 0");
		return false;
	}

	BusMsgHead head = { (unsigned int)len };

	// wr_ptr��д���ݵ�ָ��
	size_t wr_ptr = m_ctrl_.wr_ptr;
	// rd_ptr�Ƕ����ݵ�ָ��
	size_t rd_ptr = m_ctrl_.rd_ptr;

	// total_len��BusMsgHead(��Ϣͷ) + ��Ϣ��Ĵ�С
	const size_t total_len = sizeof (head)+len;

	// space��ʣ��ռ��С, size_�ǹ����ڴ�ռ��С
	const size_t space = wr_ptr >= rd_ptr ? (m_size_ - wr_ptr + rd_ptr + 1) : (rd_ptr - wr_ptr - 1);

	// ����ռ䲻��, ֱ�ӷ���, ����������Ͼ��Ǳ�������
	if (space < total_len)
	{
		LOG_ERROR("error state, space < total_len");
		return false;
	}

	// �ܵ���һ��, ֻ��˵���ܵ�ʣ��ռ��ǹ���
	// ʣ��ռ��Ϊǰ�������ֱַ��A�ռ�(���)��B�ռ�(�ұ�)
	size_t right_of_wr;

	// ��ȡд����ָ���ұ������ռ��С(>=��д�ռ�space)
	right_of_wr = m_size_ - wr_ptr;
	if (right_of_wr > sizeof (head))
	{
		// ����ռ� > sizeof(BusMsgHead), д��ͷ����
		*reinterpret_cast<BusMsgHead*> (m_base_ + wr_ptr) = head;
		// дָ��ƫ�� + sizeof(BusMsgHead)���ֽ�
		wr_ptr += sizeof (head);
	}
	else if (right_of_wr < sizeof (head))
	{
		// ����ռ� < sizeof(BusMsgHead)
		// first���ұ߿ռ�
		const size_t first = right_of_wr;
		// second����߿ռ�
		const size_t second = sizeof (head)-first;
		// ��head��һ�������ݿ������ұ߿ռ�
		memcpy(m_base_ + wr_ptr, &head, first);
		// ��head��ʣ�ಿ�����ݿ�������߿ռ�
		memcpy(m_base_, reinterpret_cast<char*> (&head) + first, second);
		// �л�дָ��λ��Ϊsecond
		wr_ptr = second;
	}
	else
	{
		// ����ռ� == sizeof(BusMsgHead), д��ͷ����
		*reinterpret_cast<BusMsgHead*> (m_base_ + wr_ptr) = head;
		// дָ��Ϊ��ʼλ��
		wr_ptr = 0;
	}

	// ԭ��ͬд��ͷ��һ����
	right_of_wr = m_size_ - wr_ptr;
	if (right_of_wr > len)
	{
		memcpy(m_base_ + wr_ptr, buf, len);
		m_ctrl_.wr_ptr = wr_ptr + len;
	}
	else if (right_of_wr < len)
	{
		const size_t first = right_of_wr;
		const size_t second = len - first;
		memcpy(m_base_ + wr_ptr, buf, first);
		memcpy(m_base_, buf + first, second);
		m_ctrl_.wr_ptr = second;
	}
	else
	{
		memcpy(m_base_ + wr_ptr, buf, len);
		m_ctrl_.wr_ptr = 0;
	}

	++m_ctrl_.wr_count;
	return true;
}


bool BusSimplexQueue::Read(char* buf, size_t len, size_t& out_len, bool peek)
{
	if (buf == 0 || len == 0)
	{
		LOG_ERROR("error para buf == 0 || len == 0");
		return false;
	}

	size_t wr_ptr = m_ctrl_.wr_ptr;
	size_t rd_ptr = m_ctrl_.rd_ptr;
	if (rd_ptr == wr_ptr)
	{
		LOG_ERROR("error state, rd_ptr == wr_ptr");
		return false;
	}

	size_t right_of_rd;

	BusMsgHead head;
	BusMsgHead* head_ptr = &head;

	right_of_rd = m_size_ - rd_ptr;//�ұ������ռ䣨<=�ɶ��ռ䣩
	if (right_of_rd > sizeof (*head_ptr))
	{
		*head_ptr = *reinterpret_cast<BusMsgHead*> (m_base_ + rd_ptr);
		rd_ptr += sizeof (*head_ptr);
	}
	else if (right_of_rd < sizeof (*head_ptr))
	{
		const size_t first = right_of_rd;
		const size_t second = sizeof (*head_ptr) - first;
		memcpy(head_ptr, m_base_ + rd_ptr, first);
		memcpy(reinterpret_cast<char*> (head_ptr)+first, m_base_, second);
		rd_ptr = second;
	}
	else
	{
		*head_ptr = *reinterpret_cast<BusMsgHead*> (m_base_ + rd_ptr);
		rd_ptr = 0;
	}

	if (len >= head_ptr->len)
	{
		len = head_ptr->len;
	}
	else
	{
		len = head_ptr->len;
		LOG_ERROR("input len not enough to write");
		return false;
	}

	right_of_rd = m_size_ - rd_ptr;
	if (right_of_rd > len)
	{
		memcpy(buf, m_base_ + rd_ptr, len);
		rd_ptr += len;
	}
	else if (right_of_rd < len)
	{
		const size_t first = right_of_rd;
		const size_t second = len - first;
		memcpy(buf, m_base_ + rd_ptr, first);
		memcpy(buf + first, m_base_, second);
		rd_ptr = second;
	}
	else
	{
		memcpy(buf, m_base_ + rd_ptr, len);
		rd_ptr = 0;
	}

	if (!peek)
	{
		m_ctrl_.rd_ptr = rd_ptr;
		++m_ctrl_.rd_count;
	}
	out_len = head_ptr->len;

	return true;
}

size_t BusSimplexQueue::MsgCount() const
{
	// ʣ���Ҫ�������Ϣ����
	// ��Ϊwr_count��rd_count������������, ��w >= rʱ, ֱ�����;
	// �� w < r, �ʹ��������, Ҫ������
	const size_t w = m_ctrl_.wr_count;
	const size_t r = m_ctrl_.rd_count;
	return w >= r ? (w - r) : (std::numeric_limits<size_t>::max() - r + w);
}



ShmQueueMgr::ShmQueueMgr()
{

}



ShmQueueMgr::~ShmQueueMgr()
{
	m_w_shm.detach();
	m_r_shm.detach();
}

bool ShmQueueMgr::Init(int write_key, int read_key, unsigned int len)
{
	if (!m_w_shm.attach(write_key, len))
	{
		LOG_DEBUG("attach shm fail, write_key=%d", write_key);
		return false;
	}
	if (!m_r_shm.attach(read_key, len))
	{
		LOG_DEBUG("attach shm fail, read_key=%d", write_key);
		return false;
	}
	m_w_queue.reset( new BusSimplexQueue((char*)m_w_shm.get_addr(), m_w_shm.get_size()) );
	m_r_queue.reset( new BusSimplexQueue((char*)m_r_shm.get_addr(), m_r_shm.get_size()) );

	return true;
}

