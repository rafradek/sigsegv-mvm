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

inline bool StringEndsWith(const char *string, const char *suffix, bool case_sensitive = true)
{
	int lengthString = strlen(string);
	int lengthSuffix = strlen(suffix);
	const char *start = string + (lengthString - lengthSuffix);
	return lengthString >= lengthSuffix && (case_sensitive ? strcmp(start, suffix) : stricmp(start, suffix)) == 0;
}

inline bool StringStartsWith(const char *string, const char *prefix, bool case_sensitive = true)
{
	return (case_sensitive ? strncmp(string, prefix, strlen(prefix)) : strnicmp(string, prefix, strlen(prefix))) == 0;
}

#define TIME_SCOPE2(name) \
	class CTimeScopeMsg_##name \
	{ \
	public: \
		CTimeScopeMsg_##name() { m_Timer.Start(); } \
		~CTimeScopeMsg_##name() \
		{ \
			m_Timer.End(); \
			Msg( #name "time: %.9fs\n", m_Timer.GetDuration().GetSeconds() ); \
		} \
	private:	\
		CFastTimer	m_Timer; \
	} name##_TSM;


#define TIME_SCOPE_DEV(name) \
	class CTimeScopeMsg_##name \
	{ \
	public: \
		CTimeScopeMsg_##name() { m_Timer.Start(); } \
		~CTimeScopeMsg_##name() \
		{ \
			m_Timer.End(); \
			DevMsg( #name "time: %.9fs\n", m_Timer.GetDuration().GetSeconds() ); \
		} \
	private:	\
		CFastTimer	m_Timer; \
	} name##_TSM;

class MultiScopeTimer
{
public:
	MultiScopeTimer(const char *name)
	{
		m_Timers.emplace_back();
		m_TimerNames.push_back(name);
		m_Timers[0].Start();
	}

	void StartNextTimer(const char *name)
	{
		m_Timers.emplace_back();
		m_TimerNames.push_back(name);
		m_Timers[m_CurrentTimer].End();
		m_Timers[++m_CurrentTimer].Start();
	}

	~MultiScopeTimer()
	{
		m_Timers[m_CurrentTimer].End();
		Msg("Timer");
		for (size_t i = 0; i < m_Timers.size(); i++) {
			Msg(", %s took %.9f s", m_TimerNames[i], m_Timers[i].GetDuration().GetSeconds());
		}
		Msg("\n");
	}

private:
	std::vector<CFastTimer> m_Timers;
	std::vector<const char*> m_TimerNames;
	int m_CurrentTimer = 0;
};

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

inline bool StringToIntStrict(const char *str, int& out, int base = 0, const char **next = nullptr)
{
	char *str_end = nullptr;
	long num = strtol(str, &str_end, base);
	
	if (next != nullptr) {
		*next = str_end;
	}

	if (str_end != str) {
		out = (int)num;
		return true;
	} else {
		return false;
	}
}

inline bool StringToIntStrictAndSpend(const char *str, int& out, int base = 0)
{
	const char *end;
	return StringToIntStrict(str, out, base, &end) && (end == nullptr || *end == '\0'); 
}

inline bool StringToFloatStrict(const char *str, float& out, const char **next = nullptr)
{
	char *str_end = nullptr;
	float num = strtof(str, &str_end);
	
	if (next != nullptr) {
		*next = str_end;
	}
	
	if (str_end != str) {
		out = num;
		return true;
	} else {
		return false;
	}
}

inline bool StringToFloatStrictAndSpend(const char *str, float& out)
{
	const char *end;
	return StringToFloatStrict(str, out, &end) && (end == nullptr || *end == '\0'); 
}

inline bool IsStrLower(const char *pch)
{
	const char *pCurrent = pch;
	while ( *pCurrent != '\0' )
	{
		if ( *pCurrent >= 'A' && *pCurrent <= 'Z' )
			return false;

		pCurrent++;
	}

	return true;
}

inline void StrLowerCopy(const char *in, char *out)
{
	char c;
	while(true) {
		c = *(in++);
		if (c >= 'A' && c <= 'Z') {
			*out = c + 32; 
		}
		else {
			*out = c;
		}
		out++;

		if (c == '\0') break;
	}
}

inline void StrLowerCopy(const char *in, char *out, size_t len)
{
	char c;
	size_t i = 0;
	while(i < len) {
		c = *(in++);
		if (c >= 'A' && c <= 'Z') {
			*out = c + 32; 
		}
		else {
			*out = c;
		}
		out++;

		if (c == '\0') break;
		i++;
	}
}

inline bool NamesMatchCaseSensitve(const char *pszQuery, string_t nameToMatch)
{
	if ( nameToMatch == NULL_STRING )
		return (!pszQuery || *pszQuery == 0 || *pszQuery == '*');

	const char *pszNameToMatch = STRING(nameToMatch);

	while ( *pszNameToMatch && *pszQuery )
	{
		unsigned char cName = *pszNameToMatch;
		unsigned char cQuery = *pszQuery;
		// simple ascii case conversion
		if (cName != cQuery) break;
		++pszNameToMatch;
		++pszQuery;
	}

	if ( *pszQuery == 0 && *pszNameToMatch == 0 )
		return true;

	// @TODO (toml 03-18-03): Perhaps support real wildcards. Right now, only thing supported is trailing *
	if ( *pszQuery == '*' )
		return true;

	return false;
}

inline bool UTIL_StringToVectorAlt(Vector &base, const char* string)
{
	int scannum = sscanf(string, "[%f %f %f]", &base[0], &base[1], &base[2]);
	if (scannum == 0)
	{
		// Try sucking out 3 floats with no []s
		scannum = sscanf(string, "%f %f %f", &base[0], &base[1], &base[2]);
	}
	return scannum == 3;
}

inline bool UTIL_StringToAnglesAlt(QAngle &base, const char* string)
{
	int scannum = sscanf(string, "[%f %f %f]", &base[0], &base[1], &base[2]);
	if (scannum == 0)
	{
		// Try sucking out 3 floats with no []s
		scannum = sscanf(string, "%f %f %f", &base[0], &base[1], &base[2]);
	}
	return scannum == 3;
}

inline bool ParseNumberOrVectorFromString(const char *str, variant_t &value)
{
    float val;
    int valint;
    Vector vec;
    if (UTIL_StringToVectorAlt(vec, str)) {
        //Msg("Parse vector\n");
        value.SetVector3D(vec);
        return true;
    }
    // Parse as int
    else if (StringToIntStrictAndSpend(str, valint)) {
       // Msg("Parse float\n");
        value.SetInt(valint);
        return true;
    }
    // Parse as float
    else if (StringToFloatStrict(str, val)) {
       // Msg("Parse float\n");
        value.SetFloat(val);
        return true;
    }
    return false;
}

inline const char *FindCaseInsensitive(const char *string, const char* needle)
{
	const char *result = nullptr;
	const char *needleadv = needle;
	char c;
	while((c = (*string++)) != '\0') {
		if ((c & ~(32)) == (*needleadv & ~(32))) {
			if (result == nullptr) {
				result = string - 1;
			}
			needleadv++;
			if (*needleadv == '\0') {
				return result;
			}
		}
		else {
			result = nullptr;
			needleadv = needle;
		}
	}
	return nullptr;
}

inline const char *FindCaseSensitive(const char *string, const char* needle)
{
	const char *result = nullptr;
	const char *needleadv = needle;
	char c;
	while((c = (*string++)) != '\0') {
		if (c == *needleadv) {
			if (result == nullptr) {
				result = string - 1;
			}
			needleadv++;
			if (*needleadv == '\0') {
				return result;
			}
		}
		else {
			result = nullptr;
			needleadv = needle;
		}
	}
	return nullptr;
}

inline const char *FindCaseSensitiveReverse(const char *string, const char* needle)
{
	size_t sizeh = strlen(string) - 1;
	size_t sizen = strlen(needle);
	const char *result = nullptr;
	const char *needleadv = needle + sizeh;
	string += sizeh;
	char c;
	const char *end = string + sizeh;
	while(end != string) {
		end--;
		c = *end;
		if (c == *needleadv) {
			if (result == nullptr) {
				result = end;
			}
			needleadv--;
			if (*needleadv == '\0') {
				return result;
			}
		}
		else {
			result = nullptr;
			needleadv = needle + sizeh;
		}
	}
	return nullptr;
}

inline const char *FindCaseInsensitiveReverse(const char *string, const char* needle)
{
	size_t sizeh = strlen(string) - 1;
	size_t sizen = strlen(needle);
	const char *result = nullptr;
	const char *needleadv = needle + sizeh;
	string += sizeh;
	char c;
	const char *end = string + sizeh;
	while(end != string) {
		end--;
		c = *end;
		if ((c & ~(32)) == ((*needleadv) & ~(32))) {
			if (result == nullptr) {
				result = end;
			}
			needleadv--;
			if (*needleadv == '\0') {
				return result;
			}
		}
		else {
			result = nullptr;
			needleadv = needle + sizeh;
		}
	}
	return nullptr;
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

void SendConVarValue(int playernum, const char *convar, const char *value);
#endif
