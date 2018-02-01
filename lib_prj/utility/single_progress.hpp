/**
author: yilaing.wu

功能：限制程序单独启动一个进程。 信号结束进程
example:
启动进程：
void main()
{
	SingleProgress::Instance().CheckSingle("my_name");
	.... //设置timer一些代码
}
void OnTimer()
{
	if (SingleProgress::Instance().IsExit())
	{
		...//处理结束
		exit(1);
	}
}

结束进程：
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

	//para sig, SIGKILL 等值
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

	//根据文件名，检查确保程序唯一进程，多次启动就会结束后启动进程
	void CheckSingle(const std::string &single_file_name);
	//请求结束唯一进程。
	void StopSingle(const std::string &single_file_name);
	//return true表示进程是退出状态。 由用户代码执行退出操作。
	//建议再timer里面不断检查这个状态，根据状态实现退出进程。
	bool IsExit(){ return m_is_exit; };

private:
	static void catch_signal(int sig_type);

private:
	bool m_is_exit;
};

