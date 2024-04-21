#ifndef _INCLUDE_SIGSEGV_SDKTF2_PLATFORMCOMPAT_H_
#define _INCLUDE_SIGSEGV_SDKTF2_PLATFORMCOMPAT_H_
#if defined(_MSC_VER)
	#define SELECTANY __declspec(selectany)
	#define RESTRICT __restrict
	#define RESTRICT_FUNC __declspec(restrict)
	#define FMTFUNCTION( a, b )
#elif defined(GNUC)
	#define SELECTANY __attribute__((weak))
	#if defined(LINUX) && !defined(DEDICATED)
		#define RESTRICT
	#else
		#define RESTRICT __restrict
	#endif
	#define RESTRICT_FUNC
	// squirrel.h does a #define printf DevMsg which leads to warnings when we try
	// to use printf as the prototype format function. Using __printf__ instead.
	#define FMTFUNCTION( fmtargnumber, firstvarargnumber ) __attribute__ (( format( __printf__, fmtargnumber, firstvarargnumber )))
#else
	#define SELECTANY static
	#define RESTRICT
	#define RESTRICT_FUNC
	#define FMTFUNCTION( a, b )
#endif
#endif