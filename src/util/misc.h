#ifndef _INCLUDE_SIGSEGV_UTIL_MISC_H_
#define _INCLUDE_SIGSEGV_UTIL_MISC_H_


#include <random>


#define SIZE_CHECK(_type, _size) static_assert(sizeof(_type) == _size, "sizeof(" #_type ") == " #_size)


constexpr long double operator"" _deg(long double deg)        { return deg * (M_PI / 180.0); }
constexpr long double operator"" _deg(unsigned long long deg) { return (long double)deg * (M_PI / 180.0); }
void StrLowerASCII(char *str);

class CaseInsensitiveLess
{
public:
	bool operator()(const std::string lhs, const std::string rhs) const
	{
		return stricmp(lhs.c_str(), rhs.c_str()) < 0;
	}
};

class CaseInsensitiveCompare 
{
public:
	bool operator() (const std::string &lhs, const std::string &rhs) const
	{
		return lhs.size() == rhs.size() && stricmp(lhs.c_str(), rhs.c_str()) == 0;
	}
};

class CaseInsensitiveHash
{
public:
	std::size_t operator() (std::string str) const
	{
		StrLowerASCII(str.data());

		return std::hash<std::string>{}(str);
	}
};

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

#define AVERAGE_TIME(name) \
	static int tickLast_##name = 0; \
	static int counter_##name = 0; \
	static CCycleCount cycle_##name; \
	counter_##name++;\
	static thread_local int scope_##name = 0; \
	if (tickLast_##name + 66 < gpGlobals->tickcount || tickLast_##name > gpGlobals->tickcount ) {\
		Msg( #name "calls: %d total: %.9fs avg: %.9fs\n", counter_##name, cycle_##name.GetSeconds(), cycle_##name.GetSeconds()/counter_##name );\
		cycle_##name.Init(); \
		counter_##name = 0;\
		tickLast_##name = gpGlobals->tickcount;\
	}\
	class CTimeScopeMsg_##name \
	{ \
	public: \
		CTimeScopeMsg_##name() { ++scope_##name; m_Timer.Start(); } \
		~CTimeScopeMsg_##name() \
		{ \
			m_Timer.End(); \
			if (--scope_##name == 0) \
				cycle_##name += m_Timer.GetDuration(); \
		} \
	private:	\
		CFastTimer m_Timer;\
	} name##_TSM; \

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

inline variant_t Variant() {
	return variant_t();
}

template<class T>
inline variant_t Variant(T value);

template<>
inline variant_t Variant<bool>(bool value)
{
	variant_t var;
	var.SetBool(value);
	return var;
}

template<>
inline variant_t Variant<float>(float value)
{
	variant_t var;
	var.SetFloat(value);
	return var;
}

template<>
inline variant_t Variant<int>(int value)
{
	variant_t var;
	var.SetInt(value);
	return var;
}

template<>
inline variant_t Variant<string_t>(string_t value)
{
	variant_t var;
	var.SetString(value);
	return var;
}

template<>
inline variant_t Variant<Vector>(Vector value)
{
	variant_t var;
	var.SetVector3D(value);
	return var;
}

template<>
inline variant_t Variant<color32>(color32 value)
{
	variant_t var;
	var.SetColor32(value);
	return var;
}

template<>
inline variant_t Variant<CBaseEntity *>(CBaseEntity * value)
{
	variant_t var;
	var.SetEntity(value);
	return var;
}

template<class T>
inline T GetVariantValueConvert(variant_t &var);

template<> inline bool GetVariantValueConvert(variant_t &var) { if (var.FieldType() != FIELD_BOOLEAN) var.Convert(FIELD_BOOLEAN); return var.Bool(); }
template<> inline int GetVariantValueConvert(variant_t &var) { if (var.FieldType() != FIELD_INTEGER) var.Convert(FIELD_INTEGER); return var.Int(); }
template<> inline float GetVariantValueConvert(variant_t &var) { if (var.FieldType() != FIELD_FLOAT) var.Convert(FIELD_FLOAT); return var.Float(); }
template<> inline string_t GetVariantValueConvert(variant_t &var) { return var.StringID(); }
template<> inline const char *GetVariantValueConvert(variant_t &var) { return var.String(); }
template<> inline Vector GetVariantValueConvert(variant_t &var) { if (var.FieldType() != FIELD_VECTOR) var.Convert(FIELD_VECTOR); Vector vec; var.Vector3D(vec); return vec; }
template<> inline QAngle GetVariantValueConvert(variant_t &var) { if (var.FieldType() != FIELD_VECTOR) var.Convert(FIELD_VECTOR); QAngle ang; var.Vector3D(*((Vector *)&ang)); return ang; }
template<> inline color32 GetVariantValueConvert(variant_t &var) { if (var.FieldType() != FIELD_COLOR32) var.Convert(FIELD_COLOR32); return var.Color32(); }
template<> inline CBaseEntity *GetVariantValueConvert(variant_t &var) { if (var.FieldType() != FIELD_EHANDLE) var.Convert(FIELD_EHANDLE); return var.Entity(); }

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

static const char * ParseToInt(const char *str, int &value) 
{
	value=0;
	const char *startPos = str;
	char c = *str;
	bool neg = false;
	bool hasNum = false;
	// Skip whitespace
	while (c == ' ' || (c >= '\t' && c <= '\r')) {
		c = *(++str);
	}
	// negative / positive
	if (c == '-') {
		c = *(++str);
		neg = true;
	}
	else if (c == '+') {
		c = *(++str);
	}
	// parse digits
	while (true) {
		if (c >= '0' && c <= '9') {
			value = value * 10 + c - '0';
			hasNum = true;
		}
		else {
			break;
		}
		c = *(++str);
	}
	if (neg) {
		value *= -1;
	}
	if (!hasNum) {
		return startPos;
	}
	return str;
}

static int ParseToInt(const char *str)
{
	int value = 0;
	ParseToInt(str, value);
	return value;
}

static const char * ParseToFloat(const char *str, float &value) 
{
	value=0.0f;
	const char *startPos = str;
	char c = *str;
	bool neg = false;
	bool hasNum = false;
	bool parseDecimal = false;
	bool parseExponential = false;
	float mult = 1.0f;

	// Skip whitespace
	while (c == ' ' || (c >= '\t' && c <= '\r')) {
		c = *(++str);
	}
	// negative / positive
	if (c == '-') {
		c = *(++str);
		neg = true;
	}
	else if (c == '+') {
		c = *(++str);
	}
	// Parse digits
	while (true) {
		if (c >= '0' && c <= '9') {
			hasNum = true;
			if (parseDecimal) {
				mult *= 0.1f;
				value = value + (float)(c - '0') * mult;
			}
			else {
				value = value * 10.0f + (float)( c - '0');
			}
		}
		else if (c == '.') {
			parseDecimal = true;
		}
		else if (hasNum && c == 'e') {
			// Exponential mode
			parseExponential = true;
			c = *(++str);
			break;
		}
		else {
			break;
		}
		c = *(++str);
	}
	if (neg) {
		value *= -1.0f;
	}
	// Parse exponential
	if (parseExponential) {
		int valExp = 0;
		neg = false;
		if (*str == '-') {
			str++;
			neg = true;
		}
		else if (*str == '+') {
			str++;
		}
		while (true) {
			if (c >= '0' && c <= '9') {
				valExp = valExp * 10 + ( c - '0');
			}
			else {
				break;
			}
			c = *(++str);
		}
		if (!neg) {
			for (int i = 0; i < valExp; i++) {
				value *= 10.0f;
			}
		}
		else {
			for (int i = 0; i < valExp; i++) {
				value *= 0.1f;
			}
		}
	}
	if (!hasNum) {
		return startPos;
	}
	return str;
}

static float ParseToFloat(const char *str)
{
	float value = 0;
	ParseToFloat(str, value);
	return value;
}


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
/** 
 * Converts string to float, if it is possible
 * @return true if conversion to float was possible, false otherwise
 */
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

/** 
 * Converts string to float, if it is possible, and there are non number characters left after the parse
 * @return true if conversion to float was possible and there are no characters left after the number, false otherwise
 */
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

inline void StrLowerASCII(char *str)
{
	while(*str) {
		if (*str >= 'A' && *str <= 'Z') {
			*str = *str + 32; 
		}
		str++;
	}
}

inline bool NamesMatch(const char *pszQuery, const char *pszNameToMatch)
{
	while ( *pszNameToMatch && *pszQuery )
	{
		unsigned char cName = *pszNameToMatch;
		unsigned char cQuery = *pszQuery;
		// simple ascii case conversion
		
		if (cName > 'A' && cName < 'Z') cName &= ~(32);
		if (cQuery > 'A' && cQuery < 'Z') cQuery &= ~(32);
		if (cName != cQuery) break;
		++pszNameToMatch;
		++pszQuery;
	}

	if ( *pszQuery == 0 && *pszNameToMatch == 0 )
		return true;

	if ( *pszQuery == '*' )
		return true;

	return false;
}

inline bool NamesMatch(const char *pszQuery, string_t nameToMatch)
{
	return NamesMatch(pszQuery, STRING(nameToMatch));
}

inline bool UTIL_StringToVectorAlt(Vector &base, const char* string)
{
	if (string[0] == '[') {
		string++;
	}
	for (int i = 0; i < 3; i++) {
		const char *next = ParseToFloat(string, base[i]);
		if (next == string) {
			for (i++; i < 3; i++) {
				base[i] = 0.0f;
			}
			return false;
		}
		string = next;
	}
	return true;
}

inline bool UTIL_StringToAnglesAlt(QAngle &base, const char* string)
{
	if (string[0] == '[') {
		string++;
	}
	for (int i = 0; i < 3; i++) {
		const char *next = ParseToFloat(string, base[i]);
		if (next == string) {
			for (i++; i < 3; i++) {
				base[i] = 0.0f;
			}
			return false;
		}
		string = next;
	}
	return true;
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
	else if (*ParseToInt(str, valint) == '\0') {
	   // Msg("Parse float\n");
		value.SetInt(valint);
		return true;
	}
	// Parse as float
	else if (ParseToFloat(str, val) != str) {
	   // Msg("Parse float\n");
		value.SetFloat(val);
		return true;
	}
	return false;
}

inline const char *FindCaseInsensitive(const char *string, const char* needle)
{
	const char *result = string;
	const char *needleadv = needle;
	char c;
	char cNeedle;
	while((c = (*string++)) != '\0') {
		if (c > 'A' && c < 'Z') c &= ~(32);
		cNeedle = *needleadv++;
		if (cNeedle == '\0') {
			return result;
		}
		if (cNeedle > 'A' && cNeedle < 'Z') cNeedle &= ~(32);

		if (c == cNeedle) {
			if (result == nullptr) {
				result = string - 1;
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
	const char *result = string;
	const char *needleadv = needle;
	char c;
	char cNeedle;
	while((c = (*string++)) != '\0') {
		cNeedle = *needleadv++;
		if (cNeedle == '\0') {
			return result;
		}

		if (c == cNeedle) {
			if (result == nullptr) {
				result = string - 1;
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
	size_t sizeh = strlen(string);
	size_t sizen = strlen(needle);
	const char *result = string;
	const char *needleadv = needle + sizen;
	char c;
	char cNeedle;
	const char *end = string + sizeh;
	while(end != string) {
		c = *--end;
		cNeedle = *--needleadv;
		if (needleadv == needle) return result;

		if (c == cNeedle) {
			result = end;
		}
		else {
			result = nullptr;
			needleadv = needle + sizen;
		}
	}
	return nullptr;
}

inline const char *FindCaseInsensitiveReverse(const char *string, const char* needle)
{
	size_t sizeh = strlen(string);
	size_t sizen = strlen(needle);
	const char *result = string;
	const char *needleadv = needle + sizen;
	char c;
	char cNeedle;
	const char *end = string + sizeh;
	while(end != string) {
		c = *--end;
		if (c > 'A' && c < 'Z') c &= ~(32);
		cNeedle = *--needleadv;
		if (needleadv == needle) return result;
		if (cNeedle > 'A' && cNeedle < 'Z') cNeedle &= ~(32);

		if (c == cNeedle) {
			result = end;
		}
		else {
			result = nullptr;
			needleadv = needle + sizen;
		}
	}
	return nullptr;
}

template<class Container, class UnaryPredicate>
inline bool RemoveFirstElement(Container &container, UnaryPredicate p)
{
	for (auto it = container.begin(); it != container.end(); it++) {
		if (p(*it)) {
			container.erase(it);
			return true;
		}
	}
	return false;
}

template<class Container, class UnaryPredicate>
inline void RemoveIf(Container &container, UnaryPredicate p)
{
	for (auto it = container.rbegin(); it != container.rend();) {
		if (p(*it)) {
			it = decltype(it)(container.erase(std::next(it).base()));
		}
		else {
			it++;
		}
	}
}

enum IterateAction
{
	IT_CONTINUE,
	IT_BREAK,
	IT_REMOVE,
	IT_REMOVE_BREAK,
};

template<class Container, class Func>
inline void Iterate(Container &container, Func f)
{
	for (auto it = container.begin(); it != container.end();) {
		IterateAction action = f(*it);
		if (action == IT_CONTINUE) {
			it++;
		}
		else if (action == IT_REMOVE) {
			it = container.erase(it);
		}
		else if (action == IT_REMOVE_BREAK) {
			container.erase(it);
			return;
		}
		else {
			return;
		}
	}
}

inline int StringReplaceAll(std::string& str, const std::string& from, const std::string& to) {
	if(from.empty())
		return 0;
	size_t startPos = 0;
	int counter = 0;
	while((startPos = str.find(from, startPos)) != std::string::npos) {
		str.replace(startPos, from.length(), to);
		startPos += to.length();
		counter++;
	}
	return counter;
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

static const char *nexttoken(char *token, size_t nMaxTokenLen, const char *str, char sep)
{
	if (nMaxTokenLen < 1)
	{
		Assert(nMaxTokenLen > 0);
		return NULL;
	}

	if ((str == NULL) || (*str == '\0'))
	{
		*token = '\0';
		return(NULL);
	}

	char *pTokenLast = token + nMaxTokenLen - 1;

	//
	// Copy everything up to the first separator into the return buffer.
	// Do not include separators in the return buffer.
	//
	while ((*str != sep) && (*str != '\0') && (token < pTokenLast))
	{
		*token++ = *str++;
	}
	*token = '\0';

	//
	// Advance the pointer unless we hit the end of the input string.
	//
	if (*str == '\0')
	{
		return(str);
	}

	return(++str);
}

template <size_t maxLenInChars> inline const char *nexttoken( OUT_Z_ARRAY char (&pToken)[maxLenInChars], const char *str, char sep)
{
	return nexttoken( pToken, maxLenInChars, str, sep );
}

class EntityMatrix : public VMatrix
{
public:
	void InitFromEntity( CBaseEntity *pEntity, int iAttachment=0 );
	void InitFromEntityLocal( CBaseEntity *entity );

	inline Vector LocalToWorld( const Vector &vVec ) const
	{
		return VMul4x3( vVec );
	}

	inline Vector WorldToLocal( const Vector &vVec ) const
	{
		return VMul4x3Transpose( vVec );
	}

	inline Vector LocalToWorldRotation( const Vector &vVec ) const
	{
		return VMul3x3( vVec );
	}

	inline Vector WorldToLocalRotation( const Vector &vVec ) const
	{
		return VMul3x3Transpose( vVec );
	}
};

std::vector<std::string> BreakStringsForMultiPrint(std::string string, size_t maxSize, char breakChar);

void CancelClientMenu(CBasePlayer *player);

int FixSlotCrashPre();
void FixSlotCrashPost(int val);

void FireEyeTrace(trace_t &trace, CBaseEntity *entity, float range = 8192, int mask = MASK_SOLID, int collisionGroup = COLLISION_GROUP_NONE);


// Countdown Timer but fully inline
class CountdownTimerInline
{
public:
	CountdownTimerInline( void )
	{
		m_timestamp = -1.0f;
		m_duration = 0.0f;
	}

	void Reset( void )
	{
		m_timestamp = Now() + m_duration;
	}		

	void Start( float duration )
	{
		m_timestamp = Now() + duration;
		m_duration = duration;
	}

	void Invalidate( void )
	{
		m_timestamp = -1.0f;
	}		

	bool HasStarted( void ) const
	{
		return (m_timestamp > 0.0f);
	}

	bool IsElapsed( void ) const
	{
		return (Now() > m_timestamp);
	}

	float GetElapsedTime( void ) const
	{
		return Now() - m_timestamp + m_duration;
	}

	float GetRemainingTime( void ) const
	{
		return (m_timestamp - Now());
	}

	/// return original countdown time
	float GetCountdownDuration( void ) const
	{
		return (m_timestamp > 0.0f) ? m_duration : 0.0f;
	}

private:
	float m_duration;
	float m_timestamp;
	inline float Now( void ) const {return gpGlobals->curtime; }
};

class IntervalTimerInline
{
public:
	IntervalTimerInline( void )
	{
		m_timestamp = -1.0f;
	}

	void Reset( void )
	{
		m_timestamp = Now();
	}		

	void Start( void )
	{
		m_timestamp = Now();
	}

	void Invalidate( void )
	{
		m_timestamp = -1.0f;
	}		

	bool HasStarted( void ) const
	{
		return (m_timestamp > 0.0f);
	}

	/// if not started, elapsed time is very large
	float GetElapsedTime( void ) const
	{
		return (HasStarted()) ? (Now() - m_timestamp) : 99999.9f;
	}

	bool IsLessThen( float duration ) const
	{
		return (Now() - m_timestamp < duration) ? true : false;
	}

	bool IsGreaterThen( float duration ) const
	{
		return (Now() - m_timestamp > duration) ? true : false;
	}

private:
	float m_timestamp;
	float Now( void ) const {return gpGlobals->curtime; }
};

/** 
 * Resolve player stuck
 * @param oldScale if the player scale changed and now the player may be stuck, the scale of the player before it changed. Otherwise just use GetModelScale()
 * @param maxMove how much, relative to player size, the player can be moved. 1 - can move entire player width
 * @param enemyOnly only unstuck from enemies, not from the world
 * @return true if the player was not stuck or was unstucked properly, false if cannot unstuck
 */
bool ResolvePlayerStuck(CBasePlayer *player, float oldScale = 1.0f, float maxMove = 1.0f, bool enemyOnly = false);

/** 
 * Get current map name, excluding the workshop/ prefix and .ugc suffix
 */
string_t GetCurrentMapNoWorkshop();
#endif