#ifndef _INCLUDE_SIGSEGV_SDKTF2_STRTOOLSCOMPAT_H_
#define _INCLUDE_SIGSEGV_SDKTF2_STRTOOLSCOMPAT_H_
#define _vsnprintf vsnprintf
#define _stricmp stricmp

int V_vsnprintf( OUT_Z_CAP(maxLenInCharacters) char *pDest, int maxLenInCharacters, PRINTF_FORMAT_STRING const char *pFormat, va_list params );
template <size_t maxLenInCharacters> int V_vsprintf_safe( OUT_Z_ARRAY char (&pDest)[maxLenInCharacters], PRINTF_FORMAT_STRING const char *pFormat, va_list params ) { return V_vsnprintf( pDest, maxLenInCharacters, pFormat, params ); }

int stricmp( const char *str1, const char *str2 )
{
	// It is not uncommon to compare a string to itself. See
	// VPanelWrapper::GetPanel which does this a lot. Since stricmp
	// is expensive and pointer comparison is cheap, this simple test
	// can save a lot of cycles, and cache pollution.
	if ( str1 == str2 )
	{
		return 0;
	}
	const unsigned char *s1 = (const unsigned char*)str1;
	const unsigned char *s2 = (const unsigned char*)str2;
	for ( ; *s1; ++s1, ++s2 )
	{
		if ( *s1 != *s2 )
		{
			// in ascii char set, lowercase = uppercase | 0x20
			unsigned char c1 = *s1 | 0x20;
			unsigned char c2 = *s2 | 0x20;
			if ( c1 != c2 || (unsigned char)(c1 - 'a') > ('z' - 'a') )
			{
				// if non-ascii mismatch, fall back to CRT for locale
				if ( (c1 | c2) >= 0x80 ) return stricmp( (const char*)s1, (const char*)s2 );
				// ascii mismatch. only use the | 0x20 value if alphabetic.
				if ((unsigned char)(c1 - 'a') > ('z' - 'a')) c1 = *s1;
				if ((unsigned char)(c2 - 'a') > ('z' - 'a')) c2 = *s2;
				return c1 > c2 ? 1 : -1;
			}
		}
	}
	return *s2 ? -1 : 0;
}

int strnicmp( const char *str1, const char *str2, int n )
{
	const unsigned char *s1 = (const unsigned char*)str1;
	const unsigned char *s2 = (const unsigned char*)str2;
	for ( ; n > 0 && *s1; --n, ++s1, ++s2 )
	{
		if ( *s1 != *s2 )
		{
			// in ascii char set, lowercase = uppercase | 0x20
			unsigned char c1 = *s1 | 0x20;
			unsigned char c2 = *s2 | 0x20;
			if ( c1 != c2 || (unsigned char)(c1 - 'a') > ('z' - 'a') )
			{
				// if non-ascii mismatch, fall back to CRT for locale
				if ( (c1 | c2) >= 0x80 ) return strnicmp( (const char*)s1, (const char*)s2, n );
				// ascii mismatch. only use the | 0x20 value if alphabetic.
				if ((unsigned char)(c1 - 'a') > ('z' - 'a')) c1 = *s1;
				if ((unsigned char)(c2 - 'a') > ('z' - 'a')) c2 = *s2;
				return c1 > c2 ? 1 : -1;
			}
		}
	}
	return (n > 0 && *s2) ? -1 : 0;
}

#endif