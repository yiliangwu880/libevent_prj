/*
use example:

DebugLog g_logFile("log.txt");  //define log file name
#define LOG_DEBUG(x, ...)  g_logFile.printf(LOG_LV_DEBUG, __FILE__, __LINE__, __FUNCTION__, x, ##__VA_ARGS__);
LOG_DEBUG("abc %d", 1);

*/
#pragma once
#include "typedef.h"
#include <string>



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
