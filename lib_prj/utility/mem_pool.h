//内存池
//用法示例  
/*
h文件：
class A
{
public:
NEWCLASS(A);
DELETECLASS(A);
};

cpp文件：
MEM_POOL_SET_SIZE(A,size)

分配对象写法和系统默认方法一样，例如：
A *p = new A;  //这里就使用Neo::MemPool<A>::Instance()池分配对象了
delete p;
*/

#pragma once  
#include <vector>  
#include <algorithm>  
#include <cassert>  

#define CHECK_FREE_ERROR //定义了，会检查释放对象参数释放正确

namespace Neo
{
	//T 为要分配对象的类
	template<class T>
	class MemPool
	{
	private:    //不允许用户自己定义内存池管理对象  
		MemPool(){}
		~MemPool(void)//最后清除所有的缓存对象  
		{
			Clear();
		}
		MemPool(const MemPool& other);
	public:
		static  MemPool<T>& Instance();           //返回单件实例  

	public:
		bool SetBufferSize(unsigned int obj_num);  //设置缓冲池大小, size为对象数量
		T* Allocate();                          //分配  
		void Free(T* pT);                       //释放  
		void Clear();                           //清除所有的缓存对象  
		//获取已分配对象数
		unsigned int GetAllocateNum()
		{
			if (m_vec_free_obj.size() > m_vec_all_obj.size())
			{
				printf("logic error");
				return -1;
			}
			return m_vec_all_obj.size() - m_vec_free_obj.size();
		}

	private:
		std::vector<T*> m_vec_all_obj;      //缓存所有对象  
		std::vector<T*> m_vec_free_obj;     //未分配的对象  


	};


	//成员函数实现  
	template<class T>
	inline MemPool<T>& MemPool<T>::Instance()
	{
		static MemPool<T> obj;
		return obj;
	}

	//设置缓存池大小  
	template<class T>
	inline bool MemPool<T>::SetBufferSize(unsigned int obj_num)
	{
		if (!m_vec_all_obj.empty())
		{
			printf("can't repeated call SetBufferSize");
			return false;
		}
		m_vec_all_obj.resize(obj_num);

		for (unsigned int i = 0; i < m_vec_all_obj.size(); ++i)
		{
			m_vec_all_obj[i] = (T *)::malloc(sizeof(T));
		}
		m_vec_free_obj.resize(obj_num);
		std::copy(m_vec_all_obj.begin(), m_vec_all_obj.end(), m_vec_free_obj.begin());
		return true;
	}



	//分配内存  
	template<class T>
	inline T* MemPool<T>::Allocate()
	{
		//内存已经分配完了， 动态增长  
		if (m_vec_free_obj.empty()) {
			printf("error, mem_pool [%s] is empty. total allocate num=%d", typeid(int).name(), (int)m_vec_all_obj.size());
			for (unsigned int i = 0; i < 1000; ++i)
			{
				T *p = (T *)::malloc(sizeof(T));
				m_vec_all_obj.push_back(p);
				m_vec_free_obj.push_back(p);
			}
		}

		T* pT = m_vec_free_obj.back();
		m_vec_free_obj.pop_back();
		return pT;

	}

	//释放对象  
	template<class T>
	inline void MemPool<T>::Free(T* pT)
	{
#ifdef CHECK_FREE_ERROR 
		//数量多了，效率会很慢，以后稳定了要去掉这个代码
		//确保删除的对象在缓存中  
		assert(std::find(m_vec_all_obj.begin(), m_vec_all_obj.end(), pT) != m_vec_all_obj.end() && "object to free is not exist!!");
		//确保只删除一次  
		assert(std::find(m_vec_free_obj.begin(), m_vec_free_obj.end(), pT) == m_vec_free_obj.end() && "object has deleted already!!");
#endif
		m_vec_free_obj.push_back(pT);
	}

	//清除缓存池内容  
	template<class T>
	inline void MemPool<T>::Clear()
	{
		for (T* p : m_vec_all_obj)
		{
			::free(p);
		}
		m_vec_all_obj.clear();
		m_vec_free_obj.clear();
	}
}


//一些宏定义，方便使用  
//定义重载new  
#define NEWCLASS(className) \
void* operator new(size_t) \
{ \
return Neo::MemPool<className>::Instance().Allocate(); \
}

//定义重载delete  
#define DELETECLASS(className)\
void operator delete(void* p) \
{ \
Neo::MemPool<className>::Instance().Free(reinterpret_cast<className*>(p));\
}


//设置缓存池大小静态成员函数  
#define MEM_POOL_SET_SIZE(className,size) static bool initial##className = Neo::MemPool<className>::Instance().SetBufferSize(size);

