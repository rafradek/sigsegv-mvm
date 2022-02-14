#ifndef _INCLUDE_SIGSEGV_MEM_VIRTUAL_HOOK_H_
#define _INCLUDE_SIGSEGV_MEM_VIRTUAL_HOOK_H_


#include "abi.h"
#include "library.h"
#include "util/scope.h"

class CVirtualHook
{
public:
	/* by addr name */
	CVirtualHook(const char *class_name, const char *func_name, void *callback, void **inner_ptr) : m_pszVTableName(class_name), m_pszFuncName(func_name), m_szName(std::string(class_name) + ";" + std::string(func_name)), m_pCallback(callback), m_pInner(inner_ptr) {};
	
	virtual bool DoLoad();
	virtual void DoUnload();
	
	virtual void DoEnable();
	virtual void DoDisable();
    
    virtual const char *GetName() { return m_szName.c_str(); };

	virtual void SetInner(void *inner, void *vtable) { *m_pInner = inner; }

    bool IsLoaded() const { return this->m_bLoaded; }
	bool IsActive() const { return this->m_bEnabled; }
	
	void Toggle(bool enable) { if (this->m_bEnabled && !enable) DoDisable(); else if(!this->m_bEnabled && enable) DoEnable(); }

private:
	const char *m_pszVTableName;
	const char *m_pszFuncName;
	std::string m_szName;
	
    bool m_bEnabled = false;
    bool m_bLoaded = false;

	void **m_pFuncPtr;

	void *m_pCallback;
	void **m_pInner;

	void *m_pVTable;
    friend class CVirtualHookFunc;
};

// typedef VTableToInnerPtr = std::vector<std::pair<void *, void *>;

// class CVirtualHookInherit : public CVirtualHook
// {
// public:
// 	/* by addr name */
// 	CVirtualHookInherit(const char *class_name, const char *func_name, void *callback, VTableToInnerPtr &vtable_to_inner_ptr) : m_pszVTableName(class_name), m_pszFuncName(func_name), m_szName(std::string(class_name) + ";" + std::string(func_name)), m_pCallback(callback), m_VTableToInnerPtr(vtable_to_inner_ptr) {};
	
// 	virtual bool DoLoad() override;
// 	virtual void DoUnload() override;
	
// 	virtual void DoEnable() override;
// 	virtual void DoDisable() override;

// 	virtual void SetInner(void **inner, void *vtable) override;
	
// private:
// 	VTableToInnerPtr &m_VTableToInnerPtr;
//     friend class CVirtualHookFunc;
// };

class CVirtualHookFunc
{
public:
	CVirtualHookFunc(void **func_ptr, void *vtable) : m_pFuncPtr(func_ptr), m_pVTable(vtable) {};
	~CVirtualHookFunc();
	
	static CVirtualHookFunc& Find(void **func_ptr, void *vtable);
	
	static void CleanUp();
	
	void AddVirtualHook(CVirtualHook *hook);
	void RemoveVirtualHook(CVirtualHook *hook);

    void UnloadAll();
    void DoHook();
private:
    void **m_pFuncPtr;
    void *m_pFuncInner;
	void *m_pVTable;

    bool m_bHooked = false;

    std::vector<CVirtualHook *> m_Hooks;
    static inline std::map<void **, CVirtualHookFunc> s_FuncMap; 
};

#define VHOOK_CALL(name) (this->*Actual)

#define VHOOK_DECL(ret, name, ...) \
	class Vhook_##name \
	{ \
	public: \
		ret callback(__VA_ARGS__); \
		static ret (Vhook_##name::* Actual)(__VA_ARGS__); \
	}; \
	static CDetour *vhook_##name = nullptr; \
	ret (Vhook_##name::* Vhook_##name::Actual)(__VA_ARGS__) = nullptr; \
	ret Vhook_##name::callback(__VA_ARGS__)

#define GET_VHOOK_CALLBACK(name) GetAddrOfMemberFunc(&Vhook_##name::callback)
#define GET_VHOOK_INNERPTR(name) reinterpret_cast<void **>(&Vhook_##name::Actual)


#endif