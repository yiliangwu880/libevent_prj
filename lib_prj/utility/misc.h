/*
author: YiliangWu
���
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

    //��stl����д��
#define FOR_IT(type, ctn)\
    for(type::iterator it=(ctn).begin(); it!=(ctn).end(); ++it)
//C++11��֧��
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

	//����˽�����䣬��ֹ�ำֵ�����ƺ���
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
//��Щ��Ҫ�����ĵط����ܺ������ͣ�����������
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

//д�������������������Щ��
const unsigned int NUM_1W = 10000;
const unsigned int NUM_10W = 10*NUM_1W;
const unsigned int NUM_100W = 100*NUM_1W;
const unsigned int NUM_1000W = 1000*NUM_1W;
const unsigned int NUM_1Y = 100*NUM_100W;

//end file
