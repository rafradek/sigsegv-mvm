#include "mem/virtual_hook.h"
#include "addr/addr.h"
#include "mem/protect.h"
#include "util/rtti.h"
#include "stub/server.h"

bool CVirtualHook::DoLoad()
{
    if (this->m_bLoaded) return true;
    
    const void **pVT  = nullptr;
    const void **pVTForCalcOffset  = nullptr;
	const void *pFunc = nullptr;

    pVT = RTTI::GetVTable(this->m_pszVTableName);
    if (pVT == nullptr) {
        DevMsg("CVirtualHook::FAIL \"%s\": can't find vtable\n", this->m_pszFuncName);
        return false;
    }

    pVTForCalcOffset = RTTI::GetVTable(this->m_pszVTableNameForCalcOffset);
    if (pVTForCalcOffset == nullptr) {
        DevMsg("CVirtualHook::FAIL \"%s\": can't find vtable\n", this->m_pszFuncName);
        return false;
    }
    
    pFunc = AddrManager::GetAddr(this->m_pszFuncName);
    if (pFunc == nullptr) {
        DevMsg("CVirtualHook::FAIL \"%s\": can't find func addr\n", this->m_pszFuncName);
        return false;
    }
    
    bool found = false;
    for (int i = 0; i < 0x1000; ++i) {
        if (pVTForCalcOffset[i] == pFunc) {
            this->m_pFuncPtr = const_cast<void **>(pVT + i);
            this->m_iOffset = i;
            found = true;
            break;
        }
    }

    // If another Virtual Hook was already applied to this function 
    if (!found) {
        this->m_pFuncPtr = CVirtualHookFunc::FindFuncPtrByOriginalFuncAndVTable(pFunc, pVT);
        if (this->m_pFuncPtr != nullptr) {
            this->m_iOffset = ((ptrdiff_t)this->m_pFuncPtr - (ptrdiff_t)pVT) / sizeof(ptrdiff_t);
            found = true;
        }
    }
    
    if (!found) {
        DevMsg("CVirtualHook::FAIL \"%s\": can't find func ptr in vtable\n", this->m_pszFuncName);
        return false;
    }
    this->m_bLoaded = true;
    this->m_pVTable = pVT;
    return true;
}

void CVirtualHook::DoEnable()
{
    CVirtualHookFunc::Find(this->m_pFuncPtr, this->m_pVTable).AddVirtualHook(this);
}

void CVirtualHook::DoDisable()
{
    CVirtualHookFunc::Find(this->m_pFuncPtr, this->m_pVTable).RemoveVirtualHook(this);
}

void CVirtualHook::DoUnload()
{
    if (this->IsActive()) {
        DoDisable();
    }
}

void *CVirtualHook::AddToVTable(void **vtable)
{
    auto oldFunc = vtable[this->m_iOffset];
    vtable[this->m_iOffset] = this->m_pCallback;
    this->SetInner(oldFunc, vtable);
    return oldFunc;
}

void CVirtualHook::Install(void *objectptr, int vtableSize)
{
    void **vtable = *((void ***)objectptr);
    void **vtableToCopy = vtable-4;

    void **newVtable = new void *[vtableSize];
    newVtable[0] = vtable;
    memcpy(newVtable+1, vtableToCopy, vtableSize-5 * sizeof(void *));
    newVtable[this->m_iOffset+5] = *this->m_pFuncPtr;
    *(void ***)objectptr = newVtable+5;
}

void CVirtualHook::Uninstall(void *objectptr)
{
    void **vtable = *((void ***)objectptr);
    void **origTable = *((void ***)vtable-5);
    delete vtable;
    *((void ***)objectptr) = origTable;
}

void CVirtualHook::ChangeVTable(void **newVT)
{
    bool wasEnabled = this->m_bEnabled;
    if (wasEnabled) {
        this->DoDisable();
    }
    
    this->m_pVTable = newVT;
    this->m_pFuncPtr = const_cast<void **>(newVT + this->m_iOffset);
    if (wasEnabled) {
        this->DoEnable();
    }
}

bool CVirtualHookAll::DoLoad()
{
    if (this->m_bLoaded) return true;
    

    const void **pVT  = nullptr;
    const void **pVTForCalcOffset  = nullptr;
	const void *pFunc = nullptr;

    
    pFunc = AddrManager::GetAddr(this->m_pszFuncName);
    if (pFunc == nullptr) {
        DevMsg("CVirtualHook::FAIL \"%s\": can't find func addr\n", this->m_pszFuncName);
        return false;
    }
    
    bool found = false;
    for (auto &[vtname, pVTInfo] : RTTI::GetAllVTableInfo()) {
        auto pVT = pVTInfo.vtable;
        for (int i = 0; i < pVTInfo.size / sizeof(void *); ++i) {
            if (pVT[i] == pFunc) {
                this->m_FoundFuncPtrAndVTablePtr.emplace(const_cast<void **>(pVT + i), pVT);
                this->m_iOffset = i;
                found = true;
                break;
            }
        }
    }

    if (!found) {
        DevMsg("CVirtualHook::FAIL \"%s\": can't find func ptr in any vtable\n", this->m_pszFuncName);
        return false;
    }

    this->m_bLoaded = true;
    this->m_pVTable = pVT;
    return true;
}

void CVirtualHookAll::DoUnload()
{
    if (this->IsActive()) {
        DoDisable();
    }
}

void CVirtualHookAll::DoEnable()
{
    if (!this->m_bEnabled && this->m_bLoaded) {
        
        for (auto &[funcPtr, pVT] : this->m_FoundFuncPtrAndVTablePtr) {
            //CVirtualHookFunc::Find(funcPtr, pVT).AddVirtualHook(this);
        }
        this->m_bEnabled = true;
    }
}

void CVirtualHookAll::DoDisable()
{
    if (this->m_bEnabled) {
        //CVirtualHookFunc::Find(this->m_pFuncPtr, this->m_pVTable).RemoveVirtualHook(this);
        this->m_bEnabled = false;
    }
}


void CVirtualHookInherit::DoEnable()
{
    if (!this->m_bEnabled && this->m_bLoaded) {
        auto origfunc = *this->m_pFuncPtr;
        for (auto &[name, vtable] : RTTI::GetAllVTable()) {
            void *result = *this->m_pFuncPtr;
            if (vtable[this->m_iOffset] == origfunc && static_cast<const std::type_info *>(*(vtable-1))->__do_upcast(*((const __cxxabiv1::__class_type_info **)this->m_pVTable-1), &result)) {
                CVirtualHookFunc::Find(this->m_pFuncPtr, (void *) *vtable).AddVirtualHook(this);
            }
        }
        
        this->m_bEnabled = true;
    }
}

void CVirtualHookInherit::DoDisable()
{
    if (this->m_bEnabled) {
        auto origfunc = *this->m_pFuncPtr;
        for (auto &[name, vtable] : RTTI::GetAllVTable()) {
            void *result = *this->m_pFuncPtr;
            if (vtable[this->m_iOffset] == origfunc && static_cast<const std::type_info *>(*(vtable-1))->__do_upcast(*((const __cxxabiv1::__class_type_info **)this->m_pVTable-1), &result)) {
                CVirtualHookFunc::Find(this->m_pFuncPtr, (void *) *vtable).RemoveVirtualHook(this);
            }
        }
        this->m_bEnabled = false;
    }
}

// bool CVirtualHookInherit::DoLoad()
// {
//     if (this->m_bLoaded) return true;
    
//     const void **pVT  = nullptr;
// 	const void *pFunc = nullptr;

//     pVT = RTTI::GetVTable(this->m_pszVTableName);
//     if (pVT == nullptr) {
//         DevMsg("CVirtualHook::FAIL \"%s\": can't find vtable\n", this->m_pszFuncName);
//         return false;
//     }
    
//     pFunc = AddrManager::GetAddr(this->m_pszFuncName);
//     if (pFunc == nullptr) {
//         DevMsg("CVirtualHook::FAIL \"%s\": can't find func addr\n", this->m_pszFuncName);
//         return false;
//     }
    
//     bool found = false;
//     for (int i = 0; i < 0x1000; ++i) {
//         if (pVT[i] == pFunc) {
//             this->m_pFuncPtr = const_cast<void **>(pVT + i);
//             found = true;
//             break;
//         }
//     }
    
//     if (!found) {
//         DevMsg("CVirtualHook::FAIL \"%s\": can't find func ptr in vtable\n", this->m_pszFuncName);
//         return false;
//     }

//     for 
//     this->m_bLoaded = true;
//     return true;
// }

CVirtualHookFunc& CVirtualHookFunc::Find(void **func_ptr, void *vtable)
{
    return s_FuncMap.try_emplace(func_ptr, func_ptr, vtable).first->second;
}

CVirtualHookFunc* CVirtualHookFunc::FindOptional(void **func_ptr)
{
    auto find = s_FuncMap.find(func_ptr);
    return find != s_FuncMap.end() ? &find->second : nullptr;
}

void **CVirtualHookFunc::FindFuncPtrByOriginalFuncAndVTable(const void *func, const void *vtable)
{
    for (auto &[func_ptr, hookFunc] : s_FuncMap) {
        if (hookFunc.m_pVTable == vtable && hookFunc.m_pFuncInner == func) {
            return hookFunc.m_pFuncPtr;
        }
    }
    return nullptr;
}

CVirtualHookFunc::~CVirtualHookFunc()
{
    UnloadAll();
}

void CVirtualHookFunc::AddVirtualHook(CVirtualHookBase *hook)
{
    this->m_Hooks.push_back(hook);
	std::sort(this->m_Hooks.begin(), this->m_Hooks.end(), [](CVirtualHookBase *&a, CVirtualHookBase *&b) {
		return a->GetPriority() > b->GetPriority();
	});
    DoHook();
    this->m_szName = this->m_Hooks.front()->GetName();
}
void CVirtualHookFunc::RemoveVirtualHook(CVirtualHookBase *hook)
{
    int prec = this->m_Hooks.size();
    this->m_Hooks.erase(std::remove(this->m_Hooks.begin(), this->m_Hooks.end(), hook), this->m_Hooks.end());
    DoHook();
    if (!this->m_Hooks.empty())
        this->m_szName = this->m_Hooks.front()->GetName();
}


void CVirtualHookFunc::CleanUp()
{
   for (auto &pair : s_FuncMap) {
       pair.second.UnloadAll();
   }
}


void CVirtualHookFunc::UnloadAll()
{
    if (this->m_bHooked) {
        // Check if some extension had overriden our hook, but not when the game is in shutdown/restart state as it does not matter by then
        int state = g_HostState.GetRef().m_currentState;
        if (this->m_pFuncFirst != *this->m_pFuncPtr && !(state == 6 || state == 7)) {
            ConColorMsg(Color(0xff, 0x20, 0x20), "[Sigsegv-MvM] Some plugin/extension had hooked a virtual %s after this extension. Those hooks are now removed as well.\n You can prevent this issue by unloading conflicting plugins before this extension\n", this->m_szName.c_str());
        }
        this->m_bHooked = false;
        MemProtModifier_RX_RWX(this->m_pFuncPtr, sizeof(void **));
        *this->m_pFuncPtr = this->m_pFuncInner;
    }
}

void CVirtualHookFunc::DoHook()
{
    UnloadAll();
    if (this->m_Hooks.empty()) return;

    this->m_bHooked = true;
    CVirtualHookBase *first = this->m_Hooks.front();
    CVirtualHookBase *last  = this->m_Hooks.back();
    this->m_pFuncInner = *this->m_pFuncPtr;

    last->SetInner(this->m_pFuncInner, this->m_pVTable);
    
    for (int i = this->m_Hooks.size() - 2; i >= 0; --i) {
        CVirtualHookBase *d1 = this->m_Hooks[i];
        CVirtualHookBase *d2 = this->m_Hooks[i + 1];
        
        d1->SetInner(d2->m_pCallback, this->m_pVTable);
    }
    MemProtModifier_RX_RWX(this->m_pFuncPtr, sizeof(void **));
    *this->m_pFuncPtr = first->m_pCallback;
    this->m_pFuncFirst = first->m_pCallback;
}