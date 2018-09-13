/*
use example:

//use defualt log
#define LOG_DEBUG(x, ...)  GetDefaultLog().printf(LOG_LV_DEBUG, __FILE__, __LINE__, __FUNCTION__, x, ##__VA_ARGS__);
LOG_DEBUG("abc %d", 1);

//custom define log file
DebugLog g_logFile("log2.txt");  //define log file name
#define LOG_DEBUG2(x, ...)  GetDefaultLog().printf(LOG_LV_DEBUG, __FILE__, __LINE__, __FUNCTION__, x, ##__VA_ARGS__);
*/
#pragma once
#include "typedef.h"
#include <string>
#include <sstream>


enum DebugLogLv
{
    //优先级从高到底
    LOG_LV_FATAL,
    LOG_LV_ERROR,
    LOG_LV_WARN,
    LOG_LV_DEBUG,
    //下面的级别，不输出文件位置信息
    LOG_LV_INFO,
    LOG_LV_ANY
};

class DebugLog 
{
public:
	static DebugLog &GetDefaultLog();
public:
    //para:const char *fname, 文件路径名
    //para:const char *prefix_name每天日志前缀字符串,通常是文件名简写
	explicit DebugLog(const char *fname, const char *prefix_name = "");
    ~DebugLog();
	void printf(DebugLogLv lv, const char * file, int line, const char *pFun, const char * pattern, ...);
    void setShowLv(DebugLogLv lv);
    //print log in std out.
    void setStdOut(bool is_std_out);
    void flush();
private:
    const char *GetLogLevelStr( DebugLogLv lv ) const;

    void print(const char * pattern, ...);
private:
    DebugLogLv m_log_lv;
    FILE *m_file;
    bool m_is_std_out;
    std::string m_prefix_name;
};


#define LOG_INFO(x, ...)  DebugLog::GetDefaultLog().printf(LOG_LV_INFO, __FILE__, __LINE__, __FUNCTION__, x, ##__VA_ARGS__);
#define LOG_DEBUG(x, ...)  DebugLog::GetDefaultLog().printf(LOG_LV_DEBUG, __FILE__, __LINE__, __FUNCTION__, x, ##__VA_ARGS__);
#define LOG_ERROR(x, ...)  DebugLog::GetDefaultLog().printf(LOG_LV_ERROR, __FILE__, __LINE__, __FUNCTION__, x, ##__VA_ARGS__);
#define LOG_WARN(x, ...)  DebugLog::GetDefaultLog().printf(LOG_LV_WARN, __FILE__, __LINE__, __FUNCTION__, x, ##__VA_ARGS__);
#define LOG_FATAL(x, ...)  DebugLog::GetDefaultLog().printf(LOG_LV_FATAL, __FILE__, __LINE__, __FUNCTION__, x, ##__VA_ARGS__);

#define LOG_CONDIT(cond, ret, x, ...)\
	do{\
	if(!(cond)){\
	DebugLog::GetDefaultLog().printf(LOG_LV_ERROR, __FILE__, __LINE__, __FUNCTION__, x, ##__VA_ARGS__); \
	return ret;\
	}	\
	}while(0)

#define LOG_CONDIT_VOID(cond, x, ...)\
	do{\
	if(!(cond))	\
	{\
	DebugLog::GetDefaultLog().printf(LOG_LV_ERROR, __FILE__, __LINE__, __FUNCTION__, x, ##__VA_ARGS__); \
		return; \
	}\
	}while(0)

#define CONDIT(cond, ret)\
	do{\
	if(!(cond)){\
	return ret;\
	}	\
	}while(0)

#define CONDIT_VOID(cond)\
	do{\
	if(!(cond))	\
	return; \
	}while(0)

#define ASSERT_DEBUG(cond)\
	do{\
	if(!(cond)){\
	DebugLog::GetDefaultLog().printf(LOG_LV_ERROR, __FILE__, __LINE__, __FUNCTION__, "assert error"); \
	}	\
	}while(0)


//for easy test
//////////////////////////////////////////////////////////////////////////

template <typename T>
void Info(T t)
{
	std::stringstream ss;
	ss << t;
	LOG_DEBUG(ss.str().c_str());
}
