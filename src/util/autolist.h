#ifndef _INCLUDE_SIGSEGV_UTIL_AUTOLIST_H_
#define _INCLUDE_SIGSEGV_UTIL_AUTOLIST_H_

#
template<typename T>
class AutoList
{
public:
	AutoList()
	{
		AllocList();
		s_List->push_back(static_cast<T *>(this));
	}
	virtual ~AutoList()
	{
		if (s_List != nullptr) {
			s_List->erase(std::find(s_List->begin(), s_List->end(), static_cast<T *>(this)));
			if (s_List->empty()) {
				delete s_List;
				s_List = nullptr;
			}
		}
	}
	
	static const std::vector<T *>& List()
	{
		AllocList();
		return *s_List;
	}
	
private:
	static void AllocList()
	{
		if (s_List == nullptr) {
			s_List = new std::vector<T *>();
		}
	}
	
	static inline std::vector<T *> *s_List = nullptr;
};

/// @brief Auto list, but does not delete the the element from the list when the object is destroyed. For use with static lifetime objects
/// @tparam T 
template<typename T>
class AutoListNoDelete
{
public:
	AutoListNoDelete()
	{
		s_List.push_back(static_cast<T *>(this));
	}
	
	static const std::vector<T *>& List()
	{
		return s_List;
	}
	
private:
	
	inline static std::vector<T *> s_List;
};

template<typename T, typename K>
class AutoMultiMap
{
public:
	AutoMultiMap(K key)
	{
		T *t = static_cast<T *>(this);
		
		AllocMultiMap();
		this->m_It = s_MultiMap->insert({key, t});
	}
	virtual ~AutoMultiMap()
	{
		if (s_MultiMap != nullptr) {
			T *t = static_cast<T *>(this);
			assert(s_MultiMap->erase(this->m_It) == 1);
			
			if (s_MultiMap->empty()) {
				delete s_MultiMap;
				s_MultiMap = nullptr;
			}
		}
	}
	
	static const std::multimap<K, T *>& MultiMap()
	{
		AllocMultiMap();
		return *s_MultiMap;
	}
	
private:
	static void AllocMultiMap()
	{
		if (s_MultiMap == nullptr) {
			s_MultiMap = new std::multimap<K, T *>();
		}
	}
	
	typename std::multimap<K, T *>::iterator m_It;
	
	static inline std::multimap<K, T *> *s_MultiMap = nullptr;
};


template<typename T>
class AutoNameMap
{
public:
	AutoNameMap()
	{
		T *t = static_cast<T *>(this);
		std::string name(t->GetName());
		
		AllocMap();
		assert(s_Map->find(name) == s_Map->end());
		(*s_Map)[name] = t;
	}
	virtual ~AutoNameMap()
	{
		if (s_Map != nullptr) {
			T *t = static_cast<T *>(this);
			std::string name(t->GetName());
			
			assert(s_Map->erase(name) == 1);
			
			if (s_Map->empty()) {
				delete s_Map;
				s_Map = nullptr;
			}
		}
	}
	
	static const std::map<std::string, T *>& Map()
	{
		AllocMap();
		return *s_Map;
	}
	
private:
	static void AllocMap()
	{
		if (s_Map == nullptr) {
			s_Map = new std::map<std::string, T *>();
		}
	}
	
	static inline std::map<std::string, T *> *s_Map = nullptr;
};


#endif
