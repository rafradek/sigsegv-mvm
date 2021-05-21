#ifndef _INCLUDE_SIGSEGV_UTIL_MISC_H_
#define _INCLUDE_SIGSEGV_UTIL_MISC_H_


#include <random>


#define SIZE_CHECK(_type, _size) static_assert(sizeof(_type) == _size, "sizeof(" #_type ") == " #_size)


constexpr long double operator"" _deg(long double deg)        { return deg * (M_PI / 180.0); }
constexpr long double operator"" _deg(unsigned long long deg) { return (long double)deg * (M_PI / 180.0); }


template<typename T,
	typename U = std::remove_reference_t<T>,
	typename = std::enable_if_t<std::is_array_v<U>>>
constexpr size_t countof()
{
	constexpr size_t extent = std::extent_v<U>;
	static_assert(extent > 0, "zero- or unknown-size array");
	
	return extent;
}
#define countof(x) countof<decltype(x)>()


template<typename T, T BASE = 10>
constexpr std::enable_if_t<std::is_integral_v<T>, int> NumDigits(T val)
{
	if (val == 0) return 1;
	
	int digits = 0;
	if (val < 0) ++digits;
	
	do {
		val /= BASE;
		++digits;
	} while (val != 0);
	
	return digits;
}


// NOTE: sadly, this conflicts with a same-named function in tier0/commonmacros.h
#if 0
template<typename T>
constexpr std::enable_if_t<std::is_unsigned_v<T>, bool> IsPowerOfTwo(T val)
{
	if (val == 0) return false;
	return (val & (val - 1)) == 0;
}
#endif

template<typename T>
constexpr std::enable_if_t<std::is_unsigned_v<T>, bool> IsMultipleOf(T val, T mult)
{
	/* avoid divide-by-zero */
	assert(mult != 0);
	
	if (val == 0) return false;
	return (val % mult == 0);
}


template<typename T>
[[nodiscard]] constexpr std::enable_if_t<std::is_unsigned_v<T>, T> RoundDownToMultiple(T val, T mult)
{
	/* avoid divide-by-zero */
	assert(mult != 0);
	
	return ((val / mult) * mult);
}

template<typename T>
[[nodiscard]] constexpr std::enable_if_t<std::is_unsigned_v<T>, T> RoundUpToMultiple(T val, T mult)
{
	/* avoid divide-by-zero and integer underflow */
	assert(mult != 0);
	/* avoid integer overflow */
	assert(val <= std::numeric_limits<T>::max() - (mult - 1));
	
	val += (mult - 1);
	return ((val / mult) * mult);
}

template<typename T>
[[nodiscard]] constexpr std::enable_if_t<std::is_unsigned_v<T>, T> RoundDownToPowerOfTwo(T val, T mult)
{
	/* avoid integer underflow */
	assert(mult != 0);
	/* verify that mult is actually a power-of-two */
	assert(IsPowerOfTwo(mult));
	
	return (val & ~(mult - 1));
}

template<typename T>
[[nodiscard]] constexpr std::enable_if_t<std::is_unsigned_v<T>, T> RoundUpToPowerOfTwo(T val, T mult)
{
	/* avoid integer underflow */
	assert(mult != 0);
	/* avoid integer overflow */
	assert(val <= std::numeric_limits<T>::max() - (mult - 1));
	/* verify that mult is actually a power-of-two */
	assert(IsPowerOfTwo(mult));
	
	val += (mult - 1);
	return (val & ~(mult - 1));
}


std::string HexDump(const void *ptr, size_t len, bool absolute = false);
void HexDumpToSpewFunc(void (*func)(const char *, ...), const void *ptr, size_t len, bool absolute = false);


/* use this when you want to do e.g. multiple calls to console spew functions
 * and don't want mat_queue_mode 2 to mess up the ordering */
class MatSingleThreadBlock
{
public:
	MatSingleThreadBlock()
	{
		if (g_pMaterialSystem != nullptr) {
			g_pMaterialSystem->AllowThreading(false, GetMaterialSystemThreadNum());
		}
	}
	~MatSingleThreadBlock()
	{
		if (g_pMaterialSystem != nullptr) {
			g_pMaterialSystem->AllowThreading(true, GetMaterialSystemThreadNum());
		}
	}
	
	bool ShouldContinue() { return (this->m_iCounter++ == 0); }
	
private:
	/* this is normally 0; with -swapcores param, it is instead 1 */
	static constexpr int GetMaterialSystemThreadNum() { return 0; }
	
	int m_iCounter = 0;
};
//#define MAT_SINGLE_THREAD_BLOCK for (MatSingleThreadBlock __mat_single_thread_block; __mat_single_thread_block.ShouldContinue(); )
#define MAT_SINGLE_THREAD_BLOCK


inline bool FStrEq(const char *sz1, const char *sz2)
{
	return (sz1 == sz2 || V_stricmp(sz1, sz2) == 0);
}


/* return an iterator to a random element in an STL container
 * based on: http://stackoverflow.com/a/16421677 */
template<typename Iterator>
Iterator select_random(Iterator begin, Iterator end)
{
	static std::random_device r_dev;
	static std::mt19937 r_engine(r_dev());
	
	std::uniform_int_distribution<> r_dist(0, std::distance(begin, end) - 1);
	
	std::advance(begin, r_dist(r_engine));
	return begin;
}

template<typename Container>
auto select_random(const Container& container)
{
	return select_random(begin(container), end(container));
}


#if 0
class CEntitySphereQuery
{
public:
	CEntitySphereQuery(const Vector& center, float radius, int flagMask = 0)
	{
		this->m_listCount = UTIL_EntitiesInSphere(this->m_pList, MAX_SPHERE_QUERY, center, radius, flagMask);
	}
	
	CBaseEntity *GetCurrentEntity()
	{
		if (this->m_listIndex >= this->m_listCount) return nullptr;
		
		return this->m_pList[this->m_listIndex];
	}
	
	void NextEntity() { ++this->m_listIndex; }
	
private:
	constexpr size_t MAX_SPHERE_QUERY = 512;
	
	int m_listIndex = 0;
	int m_listCount;
	CBaseEntity *m_pList[MAX_SPHERE_QUERY];
}
#endif


#if 0
/* allow using CHandle<T> as the key type in std::unordered_map */
namespace std
{
	template<typename T> template<> struct hash<CHandle<T>>
	{
		using argument_type = CHandle<T>;
		using result_type   = size_t;
		
		result_type operator()(const argument_type& arg) const
		{
			// TODO
		}
	};
}
#endif


struct VStricmpLess
{
	bool operator()(const char *lhs, const char *rhs) const
	{
		return (V_stricmp(lhs, rhs) < 0);
	}
};


inline bool StringToIntStrict(const char *str, int& out, int base = 0)
{
	char *str_end = nullptr;
	long num = strtol(str, &str_end, base);
	
	if (str_end != str) {
		out = (int)num;
		return true;
	} else {
		return false;
	}
}

inline bool StringToFloatStrict(const char *str, float& out)
{
	char *str_end = nullptr;
	float num = strtof(str, &str_end);
	
	if (str_end != str) {
		out = num;
		return true;
	} else {
		return false;
	}
}


template<int SIZE_BUF = FMTSTR_STD_LEN, typename... ARGS>
std::string CFmtStdStr(ARGS&&... args)
{
	return CFmtStrN<SIZE_BUF>(std::forward<ARGS>(args)...).Get();
}


template<typename T> T ConVar_GetValue(const ConVarRef& cvar);
template<> inline bool        ConVar_GetValue<bool>       (const ConVarRef& cvar) { return cvar.GetBool();   }
template<> inline int         ConVar_GetValue<int>        (const ConVarRef& cvar) { return cvar.GetInt();    }
template<> inline float       ConVar_GetValue<float>      (const ConVarRef& cvar) { return cvar.GetFloat();  }
template<> inline std::string ConVar_GetValue<std::string>(const ConVarRef& cvar) { return cvar.GetString(); }

template<typename T> void ConVar_SetValue(ConVarRef& cvar, const T& val) { cvar.SetValue(val); }
template<> inline void ConVar_SetValue(ConVarRef& cvar, const std::string& val) { cvar.SetValue(val.c_str()); }


#endif
