#include "stdafx.h"
#include "version.h"

DebugLog g_logFile("log_client.txt");  //define log file name

void test_client();
void test_httpclient();
void test_mgr_client_base();
int main(int argc, char* argv[]) 
{
	LOG_DEBUG("\n\n");
	//test_client();
	//test_httpclient();
	test_mgr_client_base();
	return 0;
}

