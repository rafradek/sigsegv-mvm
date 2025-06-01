#ifndef _INCLUDE_SIGSEGV_UTIL_TRANSLATION_H_
#define _INCLUDE_SIGSEGV_UTIL_TRANSLATION_H_

template <typename T>
T* ToPointer(T& arg) {
    return &arg;
}

template <typename T>
T* ToPointer(T* arg) {
	return arg;
}


const char *_FormatTextForPlayerSM(CBasePlayer *player, int paramCount, const char *fmt, ...);

// Format text to internal buffer, uses SourceMod formatting for %t support
template<typename ... T>
inline const char *FormatTextForPlayerSM(CBasePlayer *player, const char *fmt, const T&... args) {
    return _FormatTextForPlayerSM(player, sizeof...(args), fmt, ToPointer(args)...);
}

// Translates text for given player
const char *_TranslateText_NoParams(CBasePlayer *player, const char *name);
const char *_TranslateText_Varargs(CBasePlayer *player, const char *name, int paramCount = 0, ...);

template<typename ... T>
inline const char *_TranslateText_ToPointer(CBasePlayer *player, const char *name, const T&... args) {
	return _TranslateText_Varargs(player, name, sizeof...(args), args...);
}

// Translates text for given player. Resulting translation is stored in a static buffer, wrap them in std::string if the result needs to be saved. On failure returns translation name
template<typename ... T>
inline const char *TranslateText(CBasePlayer *player, const char *name, const T&... args) {
	return _TranslateText_ToPointer(player, name, ToPointer(args)...);
}

// Translates text for given player. The string buffer is stored until translation reload. On failure returns translation name
template<>
inline const char *TranslateText(CBasePlayer *player, const char *name) {
	return _TranslateText_NoParams(player, name);
}

void _PrintToChatSM(CBasePlayer *player, int paramCount, const char *fmt, ...);

// Prints text to chat of specified player, uses SourceMod formatting for %t support
template<typename ... T>
inline void PrintToChatSM(CBasePlayer *player, const char *fmt, const T&... args) {
    return _PrintToChatSM(player, sizeof...(args), fmt, ToPointer(args)...);
}

void _PrintToChatAllSM(int paramCount, const char *fmt, ...);

// Prints text all player chat, uses SourceMod formatting for %t support
template<typename ... T>
inline void PrintToChatAllSM(const char *fmt, const T&... args) {
    return _PrintToChatAllSM(sizeof...(args), fmt, ToPointer(args)...);
}
#endif