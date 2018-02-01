/*
author: YiliangWu
‘”œÓ£∫
)get array length
)easy use placement new
)vector fast remove an element. but change the remain elements order.
*/

#pragma once

#include <vector>
#include <list>
#include <set>
#include <map>


namespace wyl
{


//////////////////////////////////////////////////////////////////////////
	template <typename Vec>
	inline bool VecFind(const Vec& vec, typename Vec::value_type val)
	{
		typename Vec::const_iterator it = std::find(vec.begin(), vec.end(), val);
		if (it == vec.end())
		{
			return false;
		}
		return true;
	}


	template <typename Vec>
	inline bool VecRemove(Vec& vec, typename Vec::value_type val)
	{
		for(typename Vec::iterator iter = vec.begin() ; iter != vec.end(); ++iter)
		{
			if(*iter == val)
			{
				*iter = vec.back();
				vec.erase(vec.end()-1);
				return true;
			}
		}
		return false;
	}

	template <typename Vec>
	inline void VecRemoveByIndex(Vec& vec,size_t index)
	{
		vec[index] = vec[vec.size()-1];
		vec.erase(vec.end()-1);
	}

	template <typename Vec>
	inline void VecRemove(Vec& vec,typename Vec::iterator it)
	{
		*it = vec.back();
		vec.erase(vec.end()-1);
	}

	template <typename Vec>
	void VecUnique(Vec &words)
	{
		std::sort(words.begin(), words.end());
		typename Vec::iterator end_unique = unique(words.begin(), words.end());
		words.erase(end_unique, words.end());
	}
	template <typename Vec, typename AddCtn>
	void VecAppend(Vec &vec, const AddCtn &add)
	{
		vec.insert(vec.end(), add.begin(), add.end());
	}
	//easy use map
	//////////////////////////////////////////////////////////////////////////
	template <typename Map>
	inline typename Map::mapped_type *MapFind(Map &map, typename Map::key_type key)
	{
		typename Map::iterator iter = map.find(key);
		if (iter==map.end())
		{
			return NULL;
		}		
		return &(iter->second);	
	}

	template <typename Map>
	inline typename const Map::mapped_type *ConstMapFind(const Map &map, typename Map::key_type key)
	{
		typename Map::const_iterator iter = map.find(key);
		if (iter==map.end())
		{
			return NULL;
		}		
		return &(iter->second);	
	}

//////////////////////////////////////////////////////////////////////////


//simple interface of Map container
template<typename Key, typename mapped_type>
class EasyMap
{
public:
	typedef std::map<Key, mapped_type>			Map;
	typedef typename Map::iterator				iterator;
	typedef typename Map::const_iterator		const_iterator;
	typedef typename Map::key_type				key_type;
	typedef typename Map::mapped_type			mapped_type;
public:
	//return NULL when have exist element.
	inline mapped_type *insert(Key id, const mapped_type &data)
	{
		pair<Map::iterator, bool> ret = m_mapData.insert(make_pair(id,data));
		if(!ret.second)
		{
			return NULL;
		}
		return &(ret.first->second);		
	}

	inline void erase(Key id)
	{
		Map::iterator iter = m_mapData.find(id);
		if (iter!=m_mapData.end())
		{
			m_mapData.erase(id);
		}		
	}

	inline mapped_type *find(Key id)
	{
		Map::iterator iter = m_mapData.find(id);
		if (iter==m_mapData.end())
		{
			return NULL;
		}		
		return &(iter->second);	
	}

	inline iterator begin(){return m_mapData.begin();}
	inline iterator end(){return m_mapData.end();}
	inline const_iterator begin() const {return m_mapData.begin();}
	inline const_iterator end() const{return m_mapData.end();}
	inline size_t size() const{return m_mapData.size();}
private:
	Map m_mapData;
};

} //end namespace YiLiang


//end file