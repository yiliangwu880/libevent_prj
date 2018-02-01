#include "stdafx.h"
#include "version.h"

DebugLog g_logFile("log_test_bus_mgr.txt");  //define log file name

void test_client();
void test_httpclient();
void test_bus_mgr_base();
void TestFileLock();

int main(int argc, char* argv[]) 
{
	LOG_DEBUG("\n\n");
	test_bus_mgr_base();
	//TestFileLock();
	return 0;
}

