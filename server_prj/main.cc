#include "stdafx.h"
#include "version.h"

void test_server();
void test_httpsvr();
void test_mgr_svr_base();

DebugLog g_logFile("log_server.txt");  //define log file name
int main(int argc, char* argv[]) 
{
	LOG_DEBUG("\n\n");
	test_server();
	//test_httpsvr();
	//test_mgr_svr_base();
	return 0;
}

