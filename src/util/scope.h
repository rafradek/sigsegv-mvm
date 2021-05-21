#ifndef _INCLUDE_SIGSEGV_UTIL_SCOPE_H_
#define _INCLUDE_SIGSEGV_UTIL_SCOPE_H_


#include "util/backtrace.h"


class RefCount
{
public:
	operator const int&() const { return this->m_iCount; }
	
	void operator++() { this->Increment(); }
	
	void operator--() { this->Decrement(); }
	
	void Increment()
	{
		++this->m_iCount;
	}
	void Decrement(const char *file = nullptr, int line = 0)
	{
#ifdef DEBUG
		if (this->m_iCount > 0) {
			--this->m_iCount;
		} else {
			DevWarning("RefCount::Decrement: m_iCount was %d!\n", this->m_iCount);
			if (file != nullptr && line != 0) {
				DevWarning("- from %s line %d\n", file, line);
			}
			BACKTRACE();
			
			this->m_iCount = 0;
		}
#else
		--this->m_iCount;
#endif
	}
	
private:
	int m_iCount = 0;
};


class ScopedIncrement
{
public:
	ScopedIncrement(RefCount& rc, const char *file, int line) :
		m_RefCount(rc), m_strFile(file), m_iLine(line) {}
	~ScopedIncrement()
	{
		if (this->m_bIncremented) {
			this->m_RefCount.Decrement(this->m_strFile, this->m_iLine);
			this->m_bIncremented = false;
		}
	}
	
	void Increment()
	{
		
#ifdef DEBUG
		assert(!this->m_bIncremented);
#endif
		this->m_RefCount.Increment();
		this->m_bIncremented = true;
	}
	
private:
	RefCount& m_RefCount;
	const char *m_strFile;
	int m_iLine;
	
	bool m_bIncremented = false;
};
#define SCOPED_INCREMENT(rc) \
	ScopedIncrement _incr_##rc(rc, __FILE__, __LINE__); \
	_incr_##rc.Increment()
#define SCOPED_INCREMENT_IF(rc, predicate) \
	ScopedIncrement _incr_##rc(rc, __FILE__, __LINE__); \
	if (predicate) { \
		_incr_##rc.Increment(); \
	}


#if 0
class ScopedPush
{
	
};
#define SCOPED_PUSH(stack, data) stack##_push(stack, (void *)data)
#endif


#endif
