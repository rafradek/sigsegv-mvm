#ifndef _INCLUDE_SIGSEGV_MEM_SCAN_H_
#define _INCLUDE_SIGSEGV_MEM_SCAN_H_


#include "util/buf.h"


//#include "library.h"
enum class Library : int;
enum class Segment : int;


enum class ScanDir : int
{
	FORWARD,
	REVERSE,
};

enum class ScanResults : int
{
	FIRST,
	ALL,
};


// BOUNDS ======================================================================

class IBounds
{
public:
	std::pair<const uint8_t *, const uint8_t *> Get() const { return std::make_pair(this->m_Lower, this->m_Upper); }
	
protected:
	IBounds() = default;
	IBounds(const void *lower, const void *upper) :
		m_Lower(reinterpret_cast<const uint8_t *>(lower)),
		m_Upper(reinterpret_cast<const uint8_t *>(upper)) {}
	
	const uint8_t *m_Lower = nullptr;
	const uint8_t *m_Upper = nullptr;
};


class CAddrAddrBounds : public IBounds
{
public:
	CAddrAddrBounds(const void *addr1, const void *addr2) :
		IBounds(Min(addr1, addr2), Max(addr1, addr2)) {}
};


class CAddrOffBounds : public CAddrAddrBounds
{
public:
	CAddrOffBounds(const void *addr, ptrdiff_t off) :
		CAddrAddrBounds(addr, (const void *)((uintptr_t)addr + off)) {}
};


class CLibBounds : public IBounds
{
public:
	CLibBounds(Library lib);
};


class CLibSegBounds : public IBounds
{
public:
	CLibSegBounds(Library lib, Segment seg);
};


// SCANNERS ====================================================================

class IScanner
{
public:
	virtual ~IScanner() = default;
	
	ScanDir GetDir() const             { return this->m_Dir; }
	ScanResults GetResultsType() const { return this->m_RType; }
	int GetAlign() const               { return this->m_Align; }
	auto GetBounds() const             { return this->m_Bounds.Get(); }
	int GetLen() const                 { return this->m_Len; }
	
	const std::vector<const void *>& Matches() const { return this->m_Matches; }
	const void *FirstMatch() const                   { return this->m_Matches[0]; }
	bool ExactlyOneMatch() const                     { return (this->m_Matches.size() == 1); }
	
	virtual bool CheckOne(const void *where) = 0;
	
protected:
	IScanner(ScanDir dir, ScanResults rtype, int align, const IBounds& bounds, int len) :
		m_Dir(dir), m_RType(rtype), m_Align(align), m_Bounds(bounds), m_Len(len) {}
	
	void AddMatch(const void *match) { this->m_Matches.push_back(match); }
	
private:
	ScanDir m_Dir;
	ScanResults m_RType;
	int m_Align;
	IBounds m_Bounds;
	int m_Len;
	
	std::vector<const void *> m_Matches;
};


template<ScanDir DIR, ScanResults RTYPE, int ALIGN>
class CBasicScanner : public IScanner
{
public:
	CBasicScanner(const IBounds& bounds, const void *seek, int len) :
		IScanner(DIR, RTYPE, ALIGN, bounds, len)
	{
		this->m_Seek = new uint8_t[len];
		memcpy(this->m_Seek, seek, len);
	}
	virtual ~CBasicScanner()
	{
		delete[] this->m_Seek;
	}
	
	virtual bool CheckOne(const void *where) override;
	
private:
	uint8_t *m_Seek;
};

template<ScanDir DIR, ScanResults RTYPE, int ALIGN>
inline bool CBasicScanner<DIR, RTYPE, ALIGN>::CheckOne(const void *where)
{
	if (memcmp(where, this->m_Seek, this->GetLen()) == 0) {
		this->AddMatch(where);
		return true;
	} else {
		return false;
	}
}


template<ScanDir DIR, ScanResults RTYPE, typename T, int ALIGN = 1>
class CTypeScanner : public CBasicScanner<DIR, RTYPE, ALIGN>
{
public:
	CTypeScanner(const IBounds& bounds, const T& seek) :
		CBasicScanner<DIR, RTYPE, ALIGN>(bounds, reinterpret_cast<const void *>(&seek), sizeof(T)) {}
};


template<ScanDir DIR, ScanResults RTYPE, typename T>
class CAlignedTypeScanner : public CTypeScanner<DIR, RTYPE, T, sizeof(T)>
{
public:
	CAlignedTypeScanner(const IBounds& bounds, const T& seek) :
		CTypeScanner<DIR, RTYPE, T, sizeof(T)>(bounds, seek) {}
};


template<ScanDir DIR, ScanResults RTYPE, int ALIGN>
class CStringScanner : public IScanner
{
public:
	CStringScanner(const IBounds& bounds, const char *str) :
		IScanner(DIR, RTYPE, ALIGN, bounds, strlen(str) + 1), m_Str(str) {}
	
	virtual bool CheckOne(const void *where) override;
	
private:
	const char *m_Str;
};

template<ScanDir DIR, ScanResults RTYPE, int ALIGN>
inline bool CStringScanner<DIR, RTYPE, ALIGN>::CheckOne(const void *where)
{
	if (strcmp((const char *)where, this->m_Str) == 0) {
		this->AddMatch(where);
		return true;
	} else {
		return false;
	}
}


template<ScanDir DIR, ScanResults RTYPE, int ALIGN>
class CStringPrefixScanner : public IScanner
{
public:
	CStringPrefixScanner(const IBounds& bounds, const char *str) :
		IScanner(DIR, RTYPE, ALIGN, bounds, strlen(str) + 1), m_Str(str) {}
	
	virtual bool CheckOne(const void *where) override;
	
private:
	const char *m_Str;
};

template<ScanDir DIR, ScanResults RTYPE, int ALIGN>
inline bool CStringPrefixScanner<DIR, RTYPE, ALIGN>::CheckOne(const void *where)
{
	if (strncmp((const char *)where, this->m_Str, this->GetLen() - 1) == 0) {
		this->AddMatch(where);
		return true;
	} else {
		return false;
	}
}


template<ScanDir DIR, ScanResults RTYPE, int ALIGN>
class CMaskedScanner : public IScanner
{
public:
	CMaskedScanner(const IBounds& bounds, const ByteBuf& seek, const ByteBuf& mask) :
		IScanner(DIR, RTYPE, ALIGN, bounds, seek.GetSize()), m_Seek(seek), m_Mask(mask)
	{
		assert(this->m_Seek.GetSize() == this->m_Mask.GetSize());
	}
	
	virtual bool CheckOne(const void *where) override;
	
private:
	const ByteBuf& m_Seek;
	const ByteBuf& m_Mask;
};

template<ScanDir DIR, ScanResults RTYPE, int ALIGN>
inline bool CMaskedScanner<DIR, RTYPE, ALIGN>::CheckOne(const void *where)
{
	int len = this->GetLen();
	
	for (int i = 0; i < len; ++i) {
		uint8_t b_mem  = *((uint8_t *)where + i);
		uint8_t b_seek = this->m_Seek[i];
		uint8_t b_mask = this->m_Mask[i];
		
		if ((b_mem & b_mask) != (b_seek & b_mask)) {
			return false;
		}
	}
	
	this->AddMatch(where);
	return true;
}


template<ScanDir DIR, ScanResults RTYPE, int ALIGN>
class CCallScanner : public IScanner
{
public:
	CCallScanner(const IBounds& bounds, uint32_t target) :
		IScanner(DIR, RTYPE, ALIGN, bounds, sizeof(target)), m_Target(target) {}
	
	virtual bool CheckOne(const void *where) override;
	
private:
	const uint32_t m_Target;
};

template<ScanDir DIR, ScanResults RTYPE, int ALIGN>
inline bool CCallScanner<DIR, RTYPE, ALIGN>::CheckOne(const void *where)
{
	auto p_opcode = reinterpret_cast<const uint8_t *>(where);
	auto p_offset = reinterpret_cast<const uint32_t *>((uintptr_t)where + 1);
	
	if (*p_opcode == 0xe8) {
		uint32_t rel = this->m_Target - ((uintptr_t)where + 5);
		if (*p_offset == rel) {
			this->AddMatch(where);
			return true;
		}
	}
	
	return false;
}


// SCANS =======================================================================

class IScan
{
protected:
	static void RunScan(IScanner *scanner);
};


template<class SCANNER>
class CScan : public IScan
{
public:
	CScan(SCANNER&& scanner) : m_Scanner(scanner)
	{
		RunScan(&this->m_Scanner);
	}
	template<typename... ARGS>
	CScan(ARGS&&... args) : m_Scanner(std::forward<ARGS>(args)...)
	{
		RunScan(&this->m_Scanner);
	}
	
	SCANNER& GetScanner() { return this->m_Scanner; }
	
	const std::vector<const void *>& Matches() const { return this->m_Scanner.Matches(); }
	const void *FirstMatch() const                   { return this->m_Scanner.FirstMatch(); }
	bool ExactlyOneMatch() const                     { return this->m_Scanner.ExactlyOneMatch(); }
	
private:
	SCANNER m_Scanner;
};


// TODO: make CMultiScan threads globally shared/persistent instead of creating/deleting
// TODO: make heterogeneous CMultiScans possible by using RTTI

class CMultiScan : public IScan
{
public:
	template<typename CONTAINER, typename SCANNER>
	CMultiScan(const CONTAINER& scanners)
	{
		for (SCANNER& scanner : scanners) {
			this->m_Scanners.push_back(&scanner);
		}
		
		this->RunMultiScan();
	}
	template<typename... ARGS>
	CMultiScan(ARGS&&... scanners)
	{
		for (auto scanner : { (IScanner *)&scanners... }) {
			this->m_Scanners.push_back(scanner);
		}
		
		this->RunMultiScan();
	}
	
	auto& GetScanners() { return this->m_Scanners; }
	
	const std::vector<const void *>& Matches(size_t idx) const { return this->m_Scanners[idx]->Matches(); }
	const void *FirstMatch(size_t idx) const                   { return this->m_Scanners[idx]->FirstMatch(); }
	bool ExactlyOneMatch(size_t idx) const                     { return this->m_Scanners[idx]->ExactlyOneMatch(); }
	
private:
	void RunMultiScan();
	
	void ThreadWorker(int id);
	IScanner *GetTask();
	
	std::mutex m_Mutex;
	std::vector<IScanner *> m_Scanners;
	size_t m_NextIndex = 0;
};


namespace Scan
{
	const char *FindUniqueConstStr(Library lib, const char *str);
	const void *FindFuncPrologue(const void *p_in_func);
}


#endif
