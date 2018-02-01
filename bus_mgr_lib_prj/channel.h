#pragma once
#include <map>
#include "bus_proto.h"
#include <arpa/inet.h>  
#include "include_all.h"
#include "share_memory.h"
#include <memory>


class BaseChannel;
class OuterChannelClient;
class OuterChannelServer;
class InnerChannel;
class Ocsc;
class MgrConnector;

typedef int ChannelId;//==shm ps write key
typedef std::map < ChannelId, BaseChannel *> Id2Channel;

class BaseChannel
{
public:
	BaseChannel();
	virtual ~BaseChannel();

	bool Init(uint64 mc_id, const sockaddr_in &svr_addr) ;
	uint64 GetMcId() const { return m_mc_id; }
	MgrConnector *GetMc();
	ChannelId GetId() const; //m_shm_s_w.get_key()
	sockaddr_in GetSvrAddr() const { return m_svr_addr; }
	shmem &GetShmPsW(){ return m_shm_s_w; } //ps����д
	shmem &GetShmPsR(){ return m_shm_s_r; } //pc������
	void GetShareShmForPS(Proto::ShareMemory &sm) const;
	void GetShareShmForPC(Proto::ShareMemory &sm) const;

	virtual void TrySendData(){} //���Է������ݵ�����������,�ɶ�ʱ������
	virtual void OnDestoryChannel() = 0;//����������ǰ����

private:
	uint64 m_mc_id; //p ����mgr��socket����id
	sockaddr_in m_svr_addr;
	//��ps pc���ӳɹ����ٷ������ͬ�� ShmQueueMgr
	shmem m_shm_s_w; //ps����write�ĵ�ַ, ���key��ΪBaseChannel ��id
	shmem m_shm_s_r; //ps����read�ĵ�ַ

};


typedef void (*ForeachChanelCB)(BaseChannel *channel);
//����Channel������
class ChannelMgr
{
	ChannelMgr();
public:
	static ChannelMgr &Instance()
	{
		static ChannelMgr d;
		return d;
	}
	~ChannelMgr();
	void Init(int shm_key_seed){ m_shm_key_seed = shm_key_seed; };
	BaseChannel * FindChannel(ChannelId id);
	void McRevReqConnectSvr(uint64 mc_id, const sockaddr_in &svr_addr);
	OuterChannelServer *  CreateOCS(uint64 ocsc_id, uint64 mc_id, const sockaddr_in &svr_addr);
	bool FreeChannel(ChannelId id);
	int CreateShmKey();
	void ForeachAll(ForeachChanelCB cb);
	void OnMcDisconnect(uint64 mc_id);

private:
	void  CreateInnerChannel(uint64 mc_id, const sockaddr_in &svr_addr);
	bool  CreateOCC(uint64 mc_id, const sockaddr_in &svr_addr);

private:
	Id2Channel m_id_2_channel;
	int m_shm_key_seed;
};

//occc = outer channel client connector
class Occc : public BaseConnect
{
public:
	Occc(OuterChannelClient &channel);

private:
	virtual void on_recv(const MsgPack &msg) override;
	virtual void on_connected() override;
	virtual void on_disconnected() override;

private:
	OuterChannelClient &m_owner_channel;
};

//occ
//mgr ����client socket����������channel
class OuterChannelClient : public BaseChannel
{
public:
	OuterChannelClient();
	//��������Զ�̷�������ʧ�ܻ᲻���ظ����ԣ�ֱ���ɹ�
	bool Connect();
	void OnConnected();
	virtual void TrySendData() override;
	virtual void OnDestoryChannel() override;

private:

public:
	Occc m_occc;
	std::unique_ptr<ShmQueueMgr> m_shm_queue;

private:

};

//ocs
//mgr ����svr socket����������channel
class OuterChannelServer : public BaseChannel
{
public:
	OuterChannelServer();

	bool Init(uint64 oscs_id, uint64 mc_id, const sockaddr_in &svr_addr); //��д�˻����init

	virtual void TrySendData() override; 
	virtual void OnDestoryChannel() override;
public:
	std::unique_ptr<ShmQueueMgr> m_shm_queue;

private:
	Ocsc *FindOcsc();

private:
	uint64 m_ocsc_id;
};

class InnerChannel : public BaseChannel
{
public:
	InnerChannel();
	bool Init(uint64 ps_mc_id, uint64 pc_mc_id, const sockaddr_in &svr_addr);
	virtual void OnDestoryChannel() override;
private:


private:
	uint64 m_ps_mc_id; //ps ����mgr��socket����id
	uint64 m_pc_mc_id; //pc ����mgr��socket����id
};