#ifndef _INCLUDE_SIGSEGV_LINK_LINK_H_
#define _INCLUDE_SIGSEGV_LINK_LINK_H_


#include "util/autolist.h"
#include "addr/addr.h"
#include "util/rtti.h"


class ILinkage : public AutoListNoDelete<ILinkage>
{
public:
	//virtual ~ILinkage() {}
	
	void InvokeLink()
	{
		if (this->m_bLinked) return;
		
		this->m_bLinked = this->Link();
	}
	
	void ForceLink(uintptr_t addr)
	{
		this->ForceAddr(addr);
		this->m_bLinked = true;
	}
	
	bool IsLinked() const { return this->m_bLinked; }
	virtual bool ClientSide() { return false; }

	virtual const char *GetNameDebug() { return nullptr; }
	virtual const char *GetTypeDebug() { return nullptr; }
	virtual uintptr_t GetAddressDebug() { return 0; }
	
protected:
	ILinkage() {}
	
	virtual bool Link() = 0;
	virtual void ForceAddr(uintptr_t addr) = 0;
	
private:
	bool m_bLinked = false;
};


class StaticFuncThunkBase : public ILinkage
{
public:
	StaticFuncThunkBase(const char *n_func) :
		m_pszFuncName(n_func) {}

	virtual bool Link() override
	{
		if (this->m_pFuncPtr == nullptr) {
			this->m_pFuncPtr = AddrManager::GetAddr(this->m_pszFuncName);
			if (this->m_pFuncPtr == nullptr) {
				Warning("StaticFuncThunk::Link FAIL \"%s\": can't find func addr\n", this->m_pszFuncName);
				return false;
			}
		}
		
		return true;
	}

	virtual const char *GetNameDebug() { return m_pszFuncName; }
	virtual const char *GetTypeDebug() { return "STATIC_FUNC"; }
	virtual uintptr_t GetAddressDebug() { return (uintptr_t) m_pFuncPtr; }
	const void *GetFuncPtr() const { return this->m_pFuncPtr; }

private:
	virtual void ForceAddr(uintptr_t addr) override { this->m_pFuncPtr = (const void *)addr; }
	
	
	const char *m_pszFuncName;
	
	const void *m_pFuncPtr = nullptr;

	virtual bool ClientSide() override { return strnicmp(this->m_pszFuncName, "[client]", strlen("[client]")) == 0; }
};

template<typename RET, typename... PARAMS>
class StaticFuncThunk
{
public:
	using RetType = RET;
	using FPtr = RET (*)(PARAMS...);
	
	StaticFuncThunk(const char *n_func) : link(n_func) {}
	
	inline RET operator()(PARAMS... args) const
	{
#ifdef DEBUG
		assert(link.GetFuncPtr() != nullptr); 
#endif
		return (*(FPtr)link.GetFuncPtr())(args...);
	}
	bool IsLinked() const { return link.IsLinked(); }
private:
	StaticFuncThunkBase link;
};

//template<class C, typename RET, typename... PARAMS>
class MemberFuncThunkBase : public ILinkage
{
public:
	//using RetType = RET;
	
	MemberFuncThunkBase(const char *n_func) :
		m_pszFuncName(n_func) {}
	
	virtual bool Link() override
	{
		if (this->m_pFuncPtr == nullptr) {
			this->m_pFuncPtr = AddrManager::GetAddr(this->m_pszFuncName);
			if (this->m_pFuncPtr == nullptr) {
				Warning("MemberFuncThunk::Link FAIL \"%s\": can't find func addr\n", this->m_pszFuncName);
				return false;
			}
		}
		
//		DevMsg("MemberFuncThunk::Link OK 0x%08x \"%s\"\n", (uintptr_t)this->m_pFuncPtr, this->m_pszFuncName);
		return true;
	}
	
	virtual const char *GetNameDebug() { return m_pszFuncName; }
	virtual const char *GetTypeDebug() { return "MEMBER_FUNC"; }
	virtual uintptr_t GetAddressDebug() { return (uintptr_t)m_pFuncPtr; }
	const void *GetFuncPtr() const { return this->m_pFuncPtr; }
	
private:
	virtual void ForceAddr(uintptr_t addr) override { this->m_pFuncPtr = (const void *)addr; }
	
	const char *m_pszFuncName;
	
	const void *m_pFuncPtr = nullptr;

	virtual bool ClientSide() override { return strnicmp(this->m_pszFuncName, "[client]", strlen("[client]")) == 0; }
};

template<class C, typename RET, typename... PARAMS>
class MemberFuncThunk//<C, RET, PARAMS...>
{
public:
	MemberFuncThunk(const char *n_func) :
		link/*<C, RET, PARAMS...>*/(n_func) {}
private:
	MemberFuncThunkBase link;
};

template<class C, typename RET, typename... PARAMS>
class MemberFuncThunk<C *, RET, PARAMS...> //<C, RET, PARAMS...>
{
public:
	using FPtr = RET (C::*)(PARAMS...);
	
	MemberFuncThunk(const char *n_func) :
		link(n_func) {}
	
	inline RET operator()(const C *obj, PARAMS... args) const = delete;
	inline RET operator()(      C *obj, PARAMS... args) const
	{
		FPtr pFunc = MakePtrToMemberFunc<C, RET, PARAMS...>(link.GetFuncPtr());
#ifdef DEBUG
		assert(pFunc != nullptr);
		assert(obj   != nullptr);
#endif
		
		return (obj->*pFunc)(args...);
	}
private:
	MemberFuncThunkBase link;
};

template<class C, typename RET, typename... PARAMS>
class MemberFuncThunk<const C *, RET, PARAMS...>//<C, RET, PARAMS...>
{
public:
	using FPtr = RET (C::*)(PARAMS...) const;
	
	MemberFuncThunk(const char *n_func) :
		link/*<C, RET, PARAMS...>*/(n_func) {}
	
	inline RET operator()(      C *obj, PARAMS... args) const = delete;
	inline RET operator()(const C *obj, PARAMS... args) const
	{
		FPtr pFunc = MakePtrToConstMemberFunc<C, RET, PARAMS...>(link.GetFuncPtr());
#ifdef DEBUG
			assert(pFunc != nullptr);
			assert(obj   != nullptr);
#endif
		
		return (obj->*pFunc)(args...);
	}
private:
	MemberFuncThunkBase link;
};


class MemberVFuncThunkBase : public ILinkage
{
public:
	MemberVFuncThunkBase(const char *n_vtable, const char *n_func) :
		m_pszVTableName(n_vtable), m_pszFuncName(n_func) {}
	
	virtual bool Link() override
	{
		const void **pVT  = nullptr;
		const void *pFunc = nullptr;
		
		if (this->m_iVTIndex == -1) {
			pVT = RTTI::GetVTable(this->m_pszVTableName);
			if (pVT == nullptr) {
				Warning("MemberVFuncThunk::Link FAIL \"%s\": can't find vtable\n", this->m_pszFuncName);
				return false;
			}
			
			pFunc = AddrManager::GetAddr(this->m_pszFuncName);
			if (pFunc == nullptr) {
				Warning("MemberVFuncThunk::Link FAIL \"%s\": can't find func addr\n", this->m_pszFuncName);
				return false;
			}
			
			bool found = false;
			for (int i = 0; i < 0x1000; ++i) {
				if (pVT[i] == pFunc) {
					this->m_iVTIndex = i;
					found = true;
					break;
				}
			}
			
			if (!found) {
				Warning("MemberVFuncThunk::Link FAIL \"%s\": can't find func ptr in vtable\n", this->m_pszFuncName);
				return false;
			}
		}
		
//		DevMsg("MemberVFuncThunk::Link OK +0x%x \"%s\"\n", this->m_iVTIndex * 4, this->m_pszFuncName);
		return true;
	}

	virtual const char *GetNameDebug() { return m_pszFuncName; }
	virtual const char *GetTypeDebug() { return "VIRTUAL_FUNC"; }
	virtual uintptr_t GetAddressDebug() { return GetVTableIndex() * sizeof(uintptr_t); }
	int GetVTableIndex() const { return this->m_iVTIndex; }
	
private:
	virtual void ForceAddr(uintptr_t addr) override { this->m_iVTIndex = addr; }
	
	const char *m_pszVTableName;
	const char *m_pszFuncName;
	
	int m_iVTIndex = -1;

	virtual bool ClientSide() override { return strnicmp(this->m_pszFuncName, "[client]", strlen("[client]")) == 0; }
};

template<class C, typename RET, typename... PARAMS>
class MemberVFuncThunk
{
public:
	using RetType = RET;
	
	MemberVFuncThunk(const char *n_vtable, const char *n_func) :
		link(n_vtable, n_func) {}
private:
	MemberVFuncThunkBase link;
};

template<class C, typename RET, typename... PARAMS>
class MemberVFuncThunk<C *, RET, PARAMS...>
{
public:
	using RetType = RET;
	using FPtr = RET (C::*)(PARAMS...);
	
	MemberVFuncThunk(const char *n_vtable, const char *n_func) :
		link(n_vtable, n_func) {}
	
	inline RET operator()(const C *obj, PARAMS... args) const = delete;
	inline RET operator()(      C *obj, PARAMS... args) const
	{
		int vt_index = link.GetVTableIndex();
		
#ifdef DEBUG
		assert(vt_index != -1);
		assert(obj != nullptr);
#endif
		
		auto pVT = *reinterpret_cast<void **const *>(obj);
		FPtr pFunc = MakePtrToMemberFunc<C, RET, PARAMS...>(pVT[vt_index]);
		return (obj->*pFunc)(args...);
	}
private:
	MemberVFuncThunkBase link;
};

template<class C, typename RET, typename... PARAMS>
class MemberVFuncThunk<const C *, RET, PARAMS...>
{
public:
	using RetType = RET;
	using FPtr = RET (C::*)(PARAMS...) const;
	
	MemberVFuncThunk(const char *n_vtable, const char *n_func) :
		link(n_vtable, n_func) {}
	
	inline RET operator()(      C *obj, PARAMS... args) const = delete;
	inline RET operator()(const C *obj, PARAMS... args) const
	{
		int vt_index = link.GetVTableIndex();
		
#ifdef DEBUG
		assert(vt_index != -1);
		assert(obj != nullptr);
#endif
		
		auto pVT = *reinterpret_cast<void **const *>(obj);
		FPtr pFunc = MakePtrToConstMemberFunc<C, RET, PARAMS...>(pVT[vt_index]);
		return (obj->*pFunc)(args...);
	}
private:
	MemberVFuncThunkBase link;
};


class GlobalThunkBase : public ILinkage
{
public:
	GlobalThunkBase(const char *n_obj) :
		m_pszObjName(n_obj) {}
	
	virtual bool Link() override
	{
		if (this->m_pObjPtr == nullptr) {
			this->m_pObjPtr = AddrManager::GetAddr(this->m_pszObjName);
			if (this->m_pObjPtr == nullptr) {
				Warning("GlobalThunk::Link FAIL \"%s\": can't find global addr\n", this->m_pszObjName);
				return false;
			}
		}
		
//		DevMsg("GlobalThunk::Link OK 0x%08x \"%s\"\n", (uintptr_t)this->m_pObjPtr, this->m_pszObjName);
		return true;
	}
	
	virtual const char *GetNameDebug() { return m_pszObjName; }
	virtual const char *GetTypeDebug() { return "GLOBAL"; }
	virtual uintptr_t GetAddressDebug() { return  (uintptr_t) this->m_pObjPtr; }
	void *m_pObjPtr = nullptr;

private:
	virtual void ForceAddr(uintptr_t addr) override { this->m_pObjPtr = (void *)addr; }
	
	const char *m_pszObjName;
	

	virtual bool ClientSide() override { return strnicmp(this->m_pszObjName, "[client]", strlen("[client]")) == 0; }
};

template<typename T>
class GlobalThunk
{
public:
	GlobalThunk(const char *n_obj) : link(n_obj) {}
	
	inline operator T&() const
	{
		
#ifdef DEBUG
		assert(this->m_pObjPtr != nullptr);
#endif
		return this->GetRef();
	}
	
	inline T& operator->() const
	{
		return this->GetRef();
	}
	
	inline T& GetRef() const { return *(T *)link.m_pObjPtr; }
	
protected:
	inline T *GetPtr() const { return (T *)link.m_pObjPtr; }
private:
	GlobalThunkBase link;
};

template<typename T>
class GlobalThunkRW : public GlobalThunk<T>
{
public:
	GlobalThunkRW(const char *n_obj) :
		GlobalThunk<T>(n_obj) {}
	
	inline T& operator=(T& that)
	{
		*this->GetPtr() = that;
		return that;
	}
};


template<size_t SIZE>
class TypeInfoThunk : public ILinkage
{
public:
	TypeInfoThunk(const char *name, uint8_t *dst) :
		m_pszName(name), m_pDest(dst) {}
	
	virtual bool Link() override
	{
		auto rtti = RTTI::GetRTTI(this->m_pszName);
		if (rtti == nullptr) {
			static_assert((SIZE % 4) == 0);
			std::fill_n((uint32_t *)m_pDest, SIZE / 4, 0xABAD1DEA);
			
			Warning("TypeInfoThunk::Link FAIL \"%s\": can't find RTTI\n", this->m_pszName);
			return false;
		}
		
		memcpy(m_pDest, rtti, SIZE);
		
		DevMsg("TypeInfoThunk::Link OK \"%s\"\n", this->m_pszName);
		return true;
	}
	
private:
	virtual void ForceAddr(uintptr_t addr) override { /* unimplemented */ }
	
	const char *m_pszName;
	uint8_t *m_pDest;
};


template<size_t SIZE>
class VTableThunk : public ILinkage
{
public:
	VTableThunk(const char *name, uint8_t *dst) :
		m_pszName(name), m_pDest(dst) {}
	
	virtual bool Link() override
	{
		auto vt = RTTI::GetVTable(this->m_pszName);
		if (vt == nullptr) {
			static_assert((SIZE % 4) == 0);
			std::fill_n((uint32_t *)m_pDest, SIZE / 4, 0xABAD1DEA);
			
			Warning("VTableThunk::Link FAIL \"%s\": can't find vtable\n", this->m_pszName);
			return false;
		}
		
#if defined __clang__
		#error TODO
#elif defined __GNUC__
		ptrdiff_t adj = -offsetof(vtable, vfptrs);
#elif defined _MSC_VER
		ptrdiff_t adj = 0;
#endif
		
		memcpy(m_pDest, (void *)((uintptr_t)vt + adj), SIZE);
		
		DevMsg("VTableThunk::Link OK \"%s\"\n", this->m_pszName);
		return true;
	}
	
private:
	virtual void ForceAddr(uintptr_t addr) override { /* unimplemented */ }
	
	const char *m_pszName;
	uint8_t *m_pDest;
};


/* for those times when you want to call a vfunc in the base class, not through
 * the vtable */
template<class C, typename RET, typename... PARAMS>
RET CallNonVirt(C *obj, const char *n_func, PARAMS... args)
{
	assert(obj != nullptr);
	
	void *addr = AddrManager::GetAddr(n_func);
	assert(addr != nullptr);
	
	auto pFunc = MakePtrToMemberFunc<C, RET, PARAMS...>(addr);
	return (obj->*pFunc)(args...);
}


/* easy way to leverage existing code to find a VT entry offset */
class VTOffFinder : public MemberVFuncThunkBase
{
public:
	VTOffFinder(const char *n_vtable, const char *n_func) :
		MemberVFuncThunkBase(n_vtable, n_func)
	{
		this->InvokeLink();
	}
	
	bool IsValid() const       { return (this->GetVTableIndex() != -1); }
	operator ptrdiff_t() const { return (this->GetVTableIndex() * 4); }
};


#endif
