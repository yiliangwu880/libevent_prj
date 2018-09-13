
#include "logFile.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string>


using namespace std;

void DebugLog::print( const char * pattern, ... )
{
	va_list vp;
	va_start(vp,pattern);
	vfprintf(m_file,pattern,vp);
	va_end(vp);
}

void DebugLog::flush()
{
	fflush(m_file);
}

void DebugLog::setStdOut( bool is_std_out )
{
	m_is_std_out = is_std_out;
}


DebugLog::DebugLog( const char *fname, const char *prefix_name )
:m_log_lv(LOG_LV_ANY)
,m_file(NULL)
,m_is_std_out(true)
,m_prefix_name(prefix_name)
{

#ifdef WIN32
    errno_t ret = fopen_s(&m_file, fname, "at");
    if (0 != ret)
    {
        ::printf("fail open file[%s]\n", fname);
    }
#else
    m_file = fopen(fname, "at");
    if (NULL == m_file)
    {
        ::printf("fail open file[%s]\n", fname);
    }
#endif
}

void DebugLog::printf( DebugLogLv lv, const char * file, int line, const char *pFun, const char * pattern, ... )
{
    if (lv > m_log_lv)
    {
        return;
    }
	char line_str[10];
    snprintf(line_str, sizeof(line_str), "%d", line);

    //add time infomation
    char time_str[1000];
	{
		time_t long_time;
		time(&long_time); 
		tm   *now;
		now = localtime(&long_time);  
		snprintf(time_str, sizeof(time_str), "[%04d-%02d-%02d %02d:%02d:%02d] ", now->tm_year + 1900, now->tm_mon+1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
	}

    string s;
    s.append(time_str);
    s.append(m_prefix_name.c_str());
    s.append(" ");
    s.append(GetLogLevelStr(lv));
    s.append(pattern);
    if (lv<=LOG_LV_DEBUG)
    {
        s.append("  --");
        s.append(file);
        s.append(":");
        s.append(line_str);
        s.append(" ");
        s.append(pFun);
    }
    s.append("\n");

	va_list vp;
	va_start(vp,pattern);	
	if (m_is_std_out)
	{
		char out_str[1000];
		vsnprintf(out_str, sizeof(out_str), s.c_str(), vp);
		fprintf(m_file, out_str); //用一次vfprintf，再用vprintf有时候有BUG， vp被 vfprintf修改了，原因未明
		::printf(out_str);
	}
	else
	{
		vfprintf(m_file, s.c_str(), vp);
	}
    va_end(vp);
   // if (lv<=LOG_LV_ERROR)
    {
	    flush();
    }

}

const char * DebugLog::GetLogLevelStr( DebugLogLv lv ) const
{
    switch (lv)
    {
    default:
        return "<unknow>";
        break;
    case LOG_LV_FATAL:
        return "<fatal> ";
        break;
    case LOG_LV_ERROR:
        return "<error> ";
        break;
    case LOG_LV_WARN:
        return "<warn>  ";
        break;
    case LOG_LV_DEBUG:
        return "<debug> ";
        break;
    case LOG_LV_INFO:
        return "<info>  ";
        break;
    case LOG_LV_ANY:
        return "<any>   ";
        break;
    }
    return "<unknow>";
}

void DebugLog::setShowLv( DebugLogLv lv )
{
    m_log_lv = lv;
}

DebugLog::~DebugLog()
{
    if (NULL != m_file)
    {
        fclose(m_file);
    }
}

DebugLog & DebugLog::GetDefaultLog()
{
	static DebugLog s_logFile("log.txt");
	return s_logFile;
}


//file end