/*
author: yiliang.wu

*/

#pragma once
#include "include_all.h"
#include <map>
#include "utility/misc.h"
#include <memory.h>

class AddrCompareLess
{
public:
	bool operator()(const sockaddr_in &left, const sockaddr_in &right)
	{
		if (left.sin_addr.s_addr == right.sin_addr.s_addr)
		{
			return left.sin_port < right.sin_port;
		}
		else
		{
			return left.sin_addr.s_addr < right.sin_addr.s_addr;
		}
	}
};



static const size_t SHM_MAX_LEN = 4 * (1 << 20); //4M,一块共享内存长度
//共享内存KEY开始缺省值
//new key从这个值开始，一直递增
static const int SHM_DEFAULT_START_KEY = 2000; 