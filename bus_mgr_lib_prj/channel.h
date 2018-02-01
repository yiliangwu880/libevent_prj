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
	shmem &GetShmPsW(){ return m_shm_s_w; } //ps用来写
	shmem &GetShmPsR(){ return m_shm_s_r; } //pc用来读
	void GetShareShmForPS(Proto::ShareMemory &sm) const;
	void GetShareShmForPC(Proto::ShareMemory &sm) const;

	virtual void TrySendData(){} //尝试发送数据到外网服务器,由定时器调用
	virtual void OnDestoryChannel() = 0;//析构本对象前调用

private:
	uint64 m_mc_id; //p 连接mgr的socket连接id
	sockaddr_in m_svr_addr;
	//等ps pc连接成功，再分配给不同的 ShmQueueMgr
	shmem m_shm_s_w; //ps用来write的地址, 这个key作为BaseChannel 的id
	shmem m_shm_s_r; //ps用来read的地址

};


typedef void (*ForeachChanelCB)(BaseChannel *channel);
//所有Channel管理器
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
//mgr 创建client socket连接外网用channel
class OuterChannelClient : public BaseChannel
{
public:
	OuterChannelClient();
	//请求连接远程服务器，失败会不断重复尝试，直到成功
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
//mgr 创建svr socket连接外网用channel
class OuterChannelServer : public BaseChannel
{
public:
	OuterChannelServer();

	bool Init(uint64 oscs_id, uint64 mc_id, const sockaddr_in &svr_addr); //重写了基类的init

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
	uint64 m_ps_mc_id; //ps 连接mgr的socket连接id
	uint64 m_pc_mc_id; //pc 连接mgr的socket连接id
};