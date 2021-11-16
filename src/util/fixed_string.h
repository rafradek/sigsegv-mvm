
#ifndef _INCLUDE_SIGSEGV_UTIL_FIXED_STRING_H_
#define _INCLUDE_SIGSEGV_UTIL_FIXED_STRING_H_

template<size_t N>
struct FixedString 
{
    char buf[N + 1]{};
    constexpr FixedString(char const* s) 
    {
        for (unsigned i = 0; i != N; ++i) buf[i] = s[i];
    }
    constexpr operator char const*() const { return buf; }
};
template<unsigned N> FixedString(char const (&)[N]) -> FixedString<N - 1>;

#endif