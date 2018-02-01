/**
*@file BacktraceInfo.cpp
*@author derekyu
*@date 2009-11-30
*@version 1.0
*@brief BacktraceInfo的实现文件
*
*	
*/

#include <execinfo.h>
#include "signal.h"
#include "BacktraceInfo.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <cxxabi.h>

#ifndef ConstArrayLen
#define ConstArrayLen(array) (sizeof(array)/sizeof((array)[0]))
#endif
using namespace std;

namespace
{
	//解析 mangling 字符串
	string demangle(const char* symbol)
	{
		size_t size;
		int status;
		char temp[256];
		char* demangled = NULL;
		//first, try to demangle a c++ name
		if (1 == sscanf(symbol, "%*[^(]%*[^_]%127[^)+]", temp)) {
			if (NULL != (demangled = abi::__cxa_demangle(temp, NULL, &size, &status))) {
				string result(demangled);
				free(demangled);
				return result;
			}
		}
		//if that didn't work, try to get a regular c symbol
		if (1 == sscanf(symbol, "%127s", temp)) {
			return temp;
		}

		//if all else fails, just return the symbol
		return symbol;
	}
}

CBacktraceInfo::CBacktraceInfo()
:s_old_SIGABRT(SIG_DFL)
,s_old_SIGSEGV(SIG_DFL)
,s_old_SIGFPE(SIG_DFL)
{

}

const char* CBacktraceInfo::GetBacktrackInfo()
{
	//获取backtrack 字符串
	void* addr_buffer[MAX_BACKTRACE_SYMBOLS_NUMBER];
	int addr_buffer_num = backtrace(addr_buffer, ConstArrayLen(addr_buffer));
	if(addr_buffer_num <=0 )
	{
		return "NO BACKTRACE.";
	}

	char **bt_str = backtrace_symbols(addr_buffer, addr_buffer_num); //里面用malloc生成返回值
	if(bt_str == NULL)
	{
		addr_buffer_num = 0;
		return "NO BACKTRACE.";
	}

	static string s_all_bt_info;
	s_all_bt_info.clear();
	s_all_bt_info = "BACKTRACE : \n";
	for(int i = 1; i < addr_buffer_num; ++i)
	{
		char szTemp[1024];
		snprintf(szTemp, ConstArrayLen(szTemp), "#%d %s\n", i - 1, demangle(bt_str[i]).c_str());
		szTemp[1024 - 1] = 0;
		s_all_bt_info+=szTemp;
	}

	free(bt_str);
	return s_all_bt_info.c_str();
}




//signal 函数用法参考http://www.kernel.org/doc/man-pages/online/pages/man2/signal.2.html
//backtrace ，backtrace_symbols函数用法参考 http://www.kernel.org/doc/man-pages/online/pages/man3/backtrace.3.html
void CBacktraceInfo::WidebrightSegvHandler(int signum) 
{
	std::string s = GetBacktrackInfo();

	if (-1 == system("date >> ./hangup_log.log"))
	{
		printf("error cmd!");
	}
	system("echo s1 >> ./hangup_log.log");
	if (s.length() > 1024 * 9)
	{
		system("info too long, truncate!!!! >> ./hangup_log.log");
		s.resize(1024 * 9);
	}
	char cmd[1024 * 10] = {};
	snprintf(cmd, sizeof(cmd), "echo -e \"signal[%d] %s\" >> ./hangup_log.log", signum, s.c_str());
	system("echo s2 >> ./hangup_log.log");
	if (-1 == system(cmd))
	{
		printf("error cmd!");
	}
	system("echo s3 >> ./hangup_log.log");
	switch (signum)
	{
	default:
		system("echo error signum. >> ./hangup_log.log");
		signal(signum, SIG_DFL);
		break;
	case SIGSEGV:
		s_old_SIGSEGV(signum);
		break;
	case SIGABRT:
		s_old_SIGABRT(signum);
		break;
	case SIGFPE:
		s_old_SIGFPE(signum);
		break;
	}
}

void CBacktraceInfo::s_WidebrightSegvHandler(int signum)
{
	CBacktraceInfo::Instance().WidebrightSegvHandler(signum);
}

void CBacktraceInfo::RegHangUpHandler()
{
	s_old_SIGSEGV = signal(SIGSEGV, s_WidebrightSegvHandler); // SIGSEGV      11       Core    Invalid memory reference
	s_old_SIGABRT = signal(SIGABRT, s_WidebrightSegvHandler); // SIGABRT       6       Core    Abort signal from
	s_old_SIGFPE = signal(SIGFPE, s_WidebrightSegvHandler); // SIGABRT     8   floating point exception 
}

