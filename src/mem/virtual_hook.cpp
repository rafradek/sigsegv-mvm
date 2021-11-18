#include "mem/virtual_hook.h"
#include "addr/addr.h"
#include "mem/protect.h"
#include "util/rtti.h"

bool CVirtualHook::DoLoad()
{
    if (this->m_bLoaded) return true;
    
    const void **pVT  = nullptr;
	const void *pFunc = nullptr;

    pVT = RTTI::GetVTable(this->m_pszVTableName);
    if (pVT == nullptr) {
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
        if (pVT[i] == pFunc) {
            this->m_pFuncPtr = const_cast<void **>(pVT + i);
            found = true;
            break;
        }
    }
    
    if (!found) {
        DevMsg("CVirtualHook::FAIL \"%s\": can't find func ptr in vtable\n", this->m_pszFuncName);
        return false;
    }
    this->m_bLoaded = true;
    return true;
}

void CVirtualHook::DoUnload()
{
    DoDisable();
}

void CVirtualHook::DoEnable()
{
    if (!this->m_bEnabled && this->m_bLoaded) {
        CVirtualHookFunc::Find(this->m_pFuncPtr).AddVirtualHook(this);
        this->m_bEnabled = true;
    }
}

void CVirtualHook::DoDisable()
{
    if (this->m_bEnabled) {
        CVirtualHookFunc::Find(this->m_pFuncPtr).RemoveVirtualHook(this);
        this->m_bEnabled = false;
    }
}

CVirtualHookFunc& CVirtualHookFunc::Find(void **func_ptr)
{
    return s_FuncMap.emplace(func_ptr, func_ptr).first->second;
}

CVirtualHookFunc::~CVirtualHookFunc()
{
    UnloadAll();
}

void CVirtualHookFunc::AddVirtualHook(CVirtualHook *hook)
{
    this->m_Hooks.push_back(hook);
    DoHook();
}
void CVirtualHookFunc::RemoveVirtualHook(CVirtualHook *hook)
{
    this->m_Hooks.erase(std::remove(this->m_Hooks.begin(), this->m_Hooks.end(), hook), this->m_Hooks.end());
    DoHook();
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
    CVirtualHook *first = this->m_Hooks.front();
    CVirtualHook *last  = this->m_Hooks.back();
    this->m_pFuncInner = *this->m_pFuncPtr;

    *last->m_pInner = this->m_pFuncInner;
    
    for (int i = this->m_Hooks.size() - 2; i >= 0; --i) {
        CVirtualHook *d1 = this->m_Hooks[i];
        CVirtualHook *d2 = this->m_Hooks[i + 1];
        
        *d1->m_pInner = d2->m_pCallback;
    }
    MemProtModifier_RX_RWX(this->m_pFuncPtr, sizeof(void **));
    *this->m_pFuncPtr = first->m_pCallback;
}