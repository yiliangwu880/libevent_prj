/**
*@brief 堆栈模块
* 通过函数 
int backtrace (void **buffer, int size);
char **backtrace_symbols (void *const *buffer, int size);
*	获取堆栈信息

编译链接选项需要加 -rdynamic

查看日志时，可以用c++filt工具解析信息，例如：
c++filt _Z18print_stack_framesi

查看地址的文件位置，例如：
addr2line -e ./zonesvr 0xbd0f18
*/

#pragma once
#include <string>

class CBacktraceInfo
{
	typedef void(*sig_t)(int);
public:
	static CBacktraceInfo &Instance()
	{
		static CBacktraceInfo d;
		return d;
	}
	CBacktraceInfo();
	/**
	*获取所有堆栈信息
	*@return 用字符串描述的堆栈信息, 不会返回NULL
	*/
	const char* GetBacktrackInfo();

	//main函数调用注册，死机就会当前目录输出堆栈日志文件（hangup_log.log）。
	void RegHangUpHandler();

private:
	void WidebrightSegvHandler(int signum);
	static void s_WidebrightSegvHandler(int signum);

private:
	const static int MAX_BACKTRACE_SYMBOLS_NUMBER = 50; //!<最多打印的堆栈层次数
	sig_t s_old_SIGABRT ;
	sig_t s_old_SIGSEGV ;
	sig_t s_old_SIGFPE  ;
};

#define BT_STR()   CBacktraceInfo::GetBacktrackInfo()

