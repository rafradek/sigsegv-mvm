#ifndef _INCLUDE_SIGSEGV_ADDR_ADDR_H_
#define _INCLUDE_SIGSEGV_ADDR_ADDR_H_


#include "library.h"
#include "util/autolist.h"


class IAddr;

class AddrManager
{
public:
	static void Load();
	
	static void *GetAddr(const char *name);
	static const char *ReverseLookup(const void *ptr);
	
	static void CC_ListAddrs(const CCommand& cmd);
	
private:
	AddrManager() {}
	
	static inline std::map<std::string, IAddr *> s_Addrs;
	static inline std::unordered_multimap<const void *, std::string> s_ReverseAddrs;
	static inline bool s_bLoading = false;
};


class IAddr : public AutoList<IAddr>
{
public:
	enum class State : int
	{
		INITIAL,
		LOADING,
		OK,
		FAIL,
	};
	
	virtual ~IAddr() {}
	
	void Init();
	
	virtual const char *GetName() const = 0;
	virtual bool FindAddrLinux(uintptr_t& addr) const { return false; }
	virtual bool FindAddrOSX(uintptr_t& addr) const   { return this->FindAddrLinux(addr); }
	virtual bool FindAddrWin(uintptr_t& addr) const   { return false; }
	
	Library GetLibrary() const   { return this->m_Library; }
	void SetLibrary(Library lib) { this->m_Library = lib; }
	
	State GetState() const { return this->m_State; }
	void *GetAddr() const  { return (void *)this->m_iAddr; }
	
protected:
	IAddr() {}
	
private:
	bool FindAddrCommon(uintptr_t& addr) const;
	
	Library m_Library = Library::SERVER;
	State m_State = State::INITIAL;
	uintptr_t m_iAddr = 0x00000000;
};


#if defined _LINUX
inline bool IAddr::FindAddrCommon(uintptr_t& addr) const { return this->FindAddrLinux(addr); }
#elif defined _OSX
inline bool IAddr::FindAddrCommon(uintptr_t& addr) const { return this->FindAddrOSX(addr); }
#elif defined _WINDOWS
inline bool IAddr::FindAddrCommon(uintptr_t& addr) const { return this->FindAddrWin(addr); }
#else
inline bool IAddr::FindAddrCommon(uintptr_t& addr) const { return false; }
#endif


#endif
