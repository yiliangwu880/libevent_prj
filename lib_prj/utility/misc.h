/*
author: YiliangWu
杂项：
)get array length
)easy use placement new
)vector fast remove an element. but change the remain elements order.
*/

#pragma once

#include <vector>
#include <list>
#include <set>
//#include <boost/foreach.hpp>

namespace wyl
{

    //简化stl迭代写法
#define FOR_IT(type, ctn)\
    for(type::iterator it=(ctn).begin(); it!=(ctn).end(); ++it)
//C++11才支持
#define FOR_IT_(ctn)\
    for(auto it=(ctn).begin(); it!=(ctn).end(); ++it)

#define CONST_FOR_IT(type, ctn)\
    for(type::const_iterator it=(ctn).begin(); it!=(ctn).end(); ++it)

#define WHILE_NUM(num)\
    for(unsigned int i=0; i<num; ++i)

#define FOR_ARRAY(a)\
	for(unsigned int i=0; i<ArrayLen(a); ++i)

#define FOR_ARRAY_REVERSE(a)\
	for(unsigned int i=ArrayLen(a); i<ArrayLen(a); --i)

	//定义私有区间，禁止类赋值，复制函数
#define DISNABLE_COPY_AND_ASSIGN(TypeName) \
    private:\
	TypeName(const TypeName&); \
	void operator=(const TypeName&)

//////////////////////////////////////////////////////////////////////////

#define IF_RET(cond, ret)\
	do{\
	if((cond))	\
	return (ret); \
	}while(0)

#define IF_RET_VOID(cond)\
	do{\
	if((cond))	\
	return; \
	}while(0)

//////////////////////////////////////////////////////////////////////////
template<typename Array>
inline size_t ArrayLen(const Array &array)
{
	return sizeof(array)/sizeof(array[0]);
}
//有些需要常量的地方不能函数类型，先用这个解决
#define ConstArrayLen(array) (sizeof(array)/sizeof((array)[0]))

//////////////////////////////////////////////////////////////////////////
template<class T> 
inline	void constructInPlace(T  *_Ptr)
{	// construct object at _Ptr with default value
	new ((void  *)_Ptr) T();
}

template<class _Ty,class _TParam> 
inline	void constructInPlace(_Ty  *_Ptr,_TParam param)
{	// construct object at _Ptr with value param
	new ((void  *)_Ptr) _Ty(param);
}

template <class T>
inline void destructInPlace(T* p)
{
	p->~T();
}

//////////////////////////////////////////////////////////////////////////


} //end namespace YiLiang

//写大数字用这个，不容易些错
const unsigned int NUM_1W = 10000;
const unsigned int NUM_10W = 10*NUM_1W;
const unsigned int NUM_100W = 100*NUM_1W;
const unsigned int NUM_1000W = 1000*NUM_1W;
const unsigned int NUM_1Y = 100*NUM_100W;

//end file
