/**
author: yilaing.wu

���ܣ����Ƴ��򵥶�����һ�����̡� �źŽ�������
example:
�������̣�
void main()
{
	SingleProgress::Instance().CheckSingle("my_name");
	.... //����timerһЩ����
}
void OnTimer()
{
	if (SingleProgress::Instance().IsExit())
	{
		...//�������
		exit(1);
	}
}

�������̣�
void main()
{
	SingleProgress::Instance().StopSingle("my_name");

}	
*/

#pragma once
#include <string>

class file_lock
{
public:
	file_lock(const std::string& file_name);
	~file_lock();

	//para sig, SIGKILL ��ֵ
	int kill(int sig);

	//return true  locked file success;
	bool lock();

	void unlock();
private:
	std::string m_pid_file;
	int m_fd;
};


class SingleProgress
{
	SingleProgress();
public:
	static SingleProgress &Instance()
	{
		static SingleProgress d;
		return d;
	}

	//�����ļ��������ȷ������Ψһ���̣���������ͻ��������������
	void CheckSingle(const std::string &single_file_name);
	//�������Ψһ���̡�
	void StopSingle(const std::string &single_file_name);
	//return true��ʾ�������˳�״̬�� ���û�����ִ���˳�������
	//������timer���治�ϼ�����״̬������״̬ʵ���˳����̡�
	bool IsExit(){ return m_is_exit; };

private:
	static void catch_signal(int sig_type);

private:
	bool m_is_exit;
};

