#ifndef _INCLUDE_SIGSEGV_MEM_VIRTUAL_HOOK_H_
#define _INCLUDE_SIGSEGV_MEM_VIRTUAL_HOOK_H_


#include "abi.h"
#include "library.h"
#include "util/scope.h"

class CVirtualHookBase
{
public:
	enum HookPriority : int
	{
		DETOUR_HOOK = -3,
		LOWEST = -2,
		LOW = -1,
		NORMAL = 0,
		HIGH = 1,
		HIGHEST = 2
	};

	CVirtualHookBase() {}

	CVirtualHookBase(void *callback, HookPriority priority = NORMAL, std::string name = ""s) : m_pCallback(callback), m_Priority(priority), m_szName(name) {}

    virtual const char *GetName() { return m_szName.c_str(); };

	virtual void SetInner(void *inner, void *vtable) { }
	
	int GetOffset() { return m_iOffset; }

	HookPriority GetPriority() { return m_Priority; }
	void SetPriority(HookPriority priority) { m_Priority = priority; }

protected:
	int m_iOffset;

	void *m_pCallback;
	HookPriority m_Priority;

	std::string m_szName;
    
	friend class CVirtualHookFunc;
};

class CVirtualHook : public CVirtualHookBase
{
public:
	/* by addr name */
	CVirtualHook(const char *class_name, const char *class_name_for_calc_offset, const char *func_name, void *callback, void **inner_ptr) : CVirtualHookBase(callback), m_pszVTableName(class_name), m_pszVTableNameForCalcOffset(class_name_for_calc_offset), m_pszFuncName(func_name), m_szName(std::string(class_name) + ";" + std::string(func_name)), m_pInner(inner_ptr) {};
	CVirtualHook(const char *class_name, const char *func_name, void *callback, void **inner_ptr) : CVirtualHook(class_name, class_name, func_name, callback, inner_ptr) {}

	virtual bool DoLoad();
	virtual void DoUnload();
	
	virtual void DoEnable();
	virtual void DoDisable();
    
	virtual void SetInner(void *inner, void *vtable) { *m_pInner = inner; }

	// Change VTable, keep offset.
	void ChangeVTable(void **newVT);

    bool IsLoaded() const { return this->m_bLoaded; }
	bool IsActive() const { return this->m_bEnabled; }
	
	void Toggle(bool enable) { if (this->m_bEnabled && !enable) DoDisable(); else if(!this->m_bEnabled && enable) DoEnable(); m_bEnabled = enable; }

	// Create a copy of a vtable for the provided object with this virtual hook installed
	void Install(void *objectptr, int vtableSize = 512);
	// Restore original vtable of specified object. The object must have install function called on them
	void Uninstall(void *objectptr);

	// Set function in vtable to this function and return old function
	void *AddToVTable(void **vtable);

protected:
	const char *m_pszVTableName;
	const char *m_pszVTableNameForCalcOffset;
	const char *m_pszFuncName;
	std::string m_szName;
	
    bool m_bEnabled = false;
    bool m_bLoaded = false;

	void **m_pFuncPtr;

	void **m_pInner;

	void *m_pVTable;
    friend class CVirtualHookFunc;
};

// Not safe until a single vhook callback can support multiple different hooks
class CVirtualHookAll
{
public:
	/* by addr name */
	CVirtualHookAll(const char *func_name, void *callback, void **inner_ptr) : m_pszFuncName(func_name), m_szName("all;" + std::string(func_name)), m_pCallback(callback), m_pInner(inner_ptr) {};

	virtual bool DoLoad();
	virtual void DoUnload();
	
	virtual void DoEnable();
	virtual void DoDisable();

    virtual const char *GetName() { return m_szName.c_str(); };

	virtual void SetInner(void *inner, void *vtable) { *m_pInner = inner; }

    bool IsLoaded() const { return this->m_bLoaded; }
	bool IsActive() const { return this->m_bEnabled; }
	
	void Toggle(bool enable) { if (this->m_bEnabled && !enable) DoDisable(); else if(!this->m_bEnabled && enable) DoEnable(); }

	int GetOffset() { return m_iOffset; }

protected:
	const char *m_pszFuncName;
	std::string m_szName;
	
    bool m_bEnabled = false;
    bool m_bLoaded = false;

	std::set<std::pair<void **, void *>> m_FoundFuncPtrAndVTablePtr;
	void **m_pFuncPtr;

	void *m_pVTable;

	int m_iOffset;

	void *m_pCallback;
	void **m_pInner;
    
	friend class CVirtualHookFunc;
};

class CVirtualHookInherit : public CVirtualHook
{
public:
	/* by addr name */
	CVirtualHookInherit(const char *class_name, const char *func_name, void *callback, void **inner_ptr) : CVirtualHook(class_name, func_name, callback, inner_ptr) {};
	
	virtual void DoEnable() override;
	virtual void DoDisable() override;
	
private:
    friend class CVirtualHookFunc;
};

class CVirtualHookFunc
{
public:
	CVirtualHookFunc(void **func_ptr, void *vtable) : m_pFuncPtr(func_ptr), m_pVTable(vtable), m_pFuncInner(*func_ptr) {};
	~CVirtualHookFunc();
	
	static CVirtualHookFunc& Find(void **func_ptr, void *vtable);
	static CVirtualHookFunc* FindOptional(void **func_ptr);

	static void **FindFuncPtrByOriginalFuncAndVTable(const void *func, const void *vtable);
	
	static void CleanUp();
	
	void AddVirtualHook(CVirtualHookBase *hook);
	void RemoveVirtualHook(CVirtualHookBase *hook);

    void UnloadAll();
    void DoHook();
private:
    void **m_pFuncPtr;
	void *m_pVTable;

    void *m_pFuncInner;
    bool m_bHooked = false;

    std::vector<CVirtualHookBase *> m_Hooks;
    static inline std::map<void **, CVirtualHookFunc> s_FuncMap; 
	void *m_pFuncFirst = nullptr;
	std::string m_szName;
};

#define VHOOK_CALL(name) (this->*Actual)

// For per object virtual hooks
#define VHOOK_CALL_OBJSPEC(name, ...) (this->*MakePtrToMemberFunc<Vhook_##name, __VA_ARGS__>((*((void ***)this)-5)[vhook_##name->GetOffset()]))

#define VHOOK_DECL(ret, name, ...) \
	class Vhook_##name \
	{ \
	public: \
		ret callback(__VA_ARGS__); \
		static ret (Vhook_##name::* Actual)(__VA_ARGS__); \
	}; \
	static CVirtualHook *vhook_##name = nullptr; \
	ret (Vhook_##name::* Vhook_##name::Actual)(__VA_ARGS__) = nullptr; \
	ret Vhook_##name::callback(__VA_ARGS__)

#define GET_VHOOK_CALLBACK(name) GetAddrOfMemberFunc(&Vhook_##name::callback)
#define GET_VHOOK_INNERPTR(name) reinterpret_cast<void **>(&Vhook_##name::Actual)


#endif