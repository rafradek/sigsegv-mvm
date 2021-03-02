#ifndef _INCLUDE_SIGSEGV_UTIL_BACKTRACE_H_
#define _INCLUDE_SIGSEGV_UTIL_BACKTRACE_H_


#if defined DEBUG && (defined _LINUX || defined _OSX)

void print_backtrace();

#define BACKTRACE() print_backtrace()

#else

#define BACKTRACE()

#endif


#endif
