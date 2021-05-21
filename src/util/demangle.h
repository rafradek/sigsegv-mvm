#ifndef _INCLUDE_SIGSEGV_UTIL_DEMANGLE_H
#define _INCLUDE_SIGSEGV_UTIL_DEMANGLE_H


inline bool DemangleName(const char *mangled, std::string& result)
{
#if defined __clang__
	#error TODO
#elif defined __GNUC__
	constexpr int options = DMGL_GNU_V3 | DMGL_TYPES | DMGL_ANSI | DMGL_PARAMS;
	char *demangled = cplus_demangle(mangled, options);
	
	if (demangled != nullptr) {
		result = demangled;
		free(demangled);
		return true;
	} else {
		result = mangled;
		return false;
	}
#elif defined _MSC_VER
	result = mangled;
	return true;
#endif
}


inline bool DemangleTypeName(const char *mangled, std::string& result)
{
#if defined __clang__
	#error TODO
#elif defined __GNUC__
	char *prefixed = new char[strlen(mangled) + 1 + 4];
	strcpy(prefixed, "_ZTS");
	strcat(prefixed, mangled);
	
	constexpr int options = DMGL_GNU_V3 | DMGL_TYPES | DMGL_ANSI | DMGL_PARAMS;
	char *demangled = cplus_demangle(prefixed, options);
	
	delete[] prefixed;
	
	if (demangled != nullptr) {
		result = demangled;
		free(demangled);
		
		constexpr char strip[] = "typeinfo name for ";
		if (strncmp(result.c_str(), strip, strlen(strip)) == 0) {
			result = result.substr(strlen(strip));
		}
		
		return true;
	} else {
		result = mangled;
		return false;
	}
#elif defined _MSC_VER
	result = mangled;
	return true;
#endif
}

inline const char *DemangleTypeName(const std::type_info& typeinfo)
{
	const char *mangled = typeinfo.name();
	
#if defined __clang__
	#error TODO
#elif defined __GNUC__
	std::string demangled;
	if (DemangleTypeName(mangled, demangled)) {
		extern string_t AllocPooledString(const char *pszValue);
		return STRING(AllocPooledString(demangled.c_str()));
	} else {
		return mangled;
	}
#elif defined _MSC_VER
	return mangled;
#endif
}


#endif