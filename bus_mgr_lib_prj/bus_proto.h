//P和mgr通讯协议
#pragma once
#include "utility/logFile.h"
#include <arpa/inet.h>  

#pragma pack(1)
namespace Proto
{
	struct BaseCmd
	{
		uint16 cmd_id; 
	};
	struct BaseRes : public BaseCmd
	{
		uint16 error_code; //0表示无错
		char error_msg[100];
	};
	struct ShareMemory
	{
		int write_key;
		int read_key;
		unsigned int len; //两个地址有效字节数一样
	};

/////////////////////////mgr ps通讯////////////////////////////////
	
	//请求mgr注册服务器
	const uint16 req_reg_svr_cmd = 100;
	struct req_reg_svr : public BaseCmd
	{
		req_reg_svr()
		{
			cmd_id = req_reg_svr_cmd;
		}
		bool is_outer; //true表示支持outer,会在mgr创建侦听服务器
		sockaddr_in svr_addr;
	}; 

	//响应失败2种情况（重复注册，服务器S地址已经有socket使用）
	const uint16 res_reg_svr_cmd = 101;
	struct res_reg_svr : public BaseRes
	{
		res_reg_svr()
		{
			cmd_id = res_reg_svr_cmd;
		}
		sockaddr_in svr_addr;
	};

	//mgr接收到客户端连接,通知ps创建channel
	const uint16 ntf_create_ps_channel_cmd = 102;
	struct ntf_create_ps_channel : public BaseCmd
	{
		ntf_create_ps_channel()
		{
			cmd_id = ntf_create_ps_channel_cmd;
		}
		sockaddr_in svr_addr;
		ShareMemory shm;
	};

	//断开连接通知
	//mgr to ps
	const uint16 ntf_disconnect_ps_cmd = 103;
	struct ntf_disconnect_ps : public BaseCmd
	{
		ntf_disconnect_ps()
		{
			cmd_id = ntf_disconnect_ps_cmd;
		}
		int ps_write_key;
	};

	//断开连接通知
	//ps to mgr
	const uint16 pm_ntf_disconnect_ps_cmd = 104;
	struct pm_ntf_disconnect_ps : public BaseCmd
	{
		pm_ntf_disconnect_ps()
		{
			cmd_id = pm_ntf_disconnect_ps_cmd;
		}
		int ps_write_key;
	};


///////////////////////////////////////////////////////////////////////////////

/////////////////////////mgr pc通讯////////////////////////////////

	//成功会响应 ntf_connect_svr_cmd， 失败会收到ntf_disconnect_pc_cmd
	const uint16 req_connect_svr_cmd = 114;
	struct req_connect_svr : public BaseCmd
	{
		req_connect_svr()
		{
			cmd_id = req_connect_svr_cmd;
		}
		sockaddr_in svr_addr;
	};

	const uint16 ntf_connect_svr_cmd = 115;
	struct ntf_connect_svr : public BaseRes
	{
		ntf_connect_svr()
		{
			cmd_id = ntf_connect_svr_cmd;
		}
		sockaddr_in svr_addr;
		ShareMemory shm;
	};

	//断开连接通知, 包括请求失败或者中途被断开
	//mgr to pc
	const uint16 ntf_disconnect_pc_cmd = 116;
	struct ntf_disconnect_pc : public BaseCmd
	{
		ntf_disconnect_pc()
		{
			cmd_id = ntf_disconnect_pc_cmd;
		}
		sockaddr_in svr_addr;
	};

	//断开连接通知
	//pc to mgr
	const uint16 pm_ntf_disconnect_pc_cmd = 117;
	struct pm_ntf_disconnect_pc : public BaseCmd
	{
		pm_ntf_disconnect_pc()
		{
			cmd_id = pm_ntf_disconnect_pc_cmd;
		}
		sockaddr_in svr_addr;
		int ps_write_key;
	};

///////////////////////////////////////////////////////////////////////////////



}//namespace Proto;

#pragma pack()