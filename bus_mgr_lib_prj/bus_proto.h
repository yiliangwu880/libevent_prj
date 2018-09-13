//P��mgrͨѶЭ��
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
		uint16 error_code; //0��ʾ�޴�
		char error_msg[100];
	};
	struct ShareMemory
	{
		int write_key;
		int read_key;
		unsigned int len; //������ַ��Ч�ֽ���һ��
	};

/////////////////////////mgr psͨѶ////////////////////////////////
	
	//����mgrע�������
	const uint16 req_reg_svr_cmd = 100;
	struct req_reg_svr : public BaseCmd
	{
		req_reg_svr()
		{
			cmd_id = req_reg_svr_cmd;
		}
		bool is_outer; //true��ʾ֧��outer,����mgr��������������
		sockaddr_in svr_addr;
	}; 

	//��Ӧʧ��2��������ظ�ע�ᣬ������S��ַ�Ѿ���socketʹ�ã�
	const uint16 res_reg_svr_cmd = 101;
	struct res_reg_svr : public BaseRes
	{
		res_reg_svr()
		{
			cmd_id = res_reg_svr_cmd;
		}
		sockaddr_in svr_addr;
	};

	//mgr���յ��ͻ�������,֪ͨps����channel
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

	//�Ͽ�����֪ͨ
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

	//�Ͽ�����֪ͨ
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

/////////////////////////mgr pcͨѶ////////////////////////////////

	//�ɹ�����Ӧ ntf_connect_svr_cmd�� ʧ�ܻ��յ�ntf_disconnect_pc_cmd
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

	//�Ͽ�����֪ͨ, ��������ʧ�ܻ�����;���Ͽ�
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

	//�Ͽ�����֪ͨ
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