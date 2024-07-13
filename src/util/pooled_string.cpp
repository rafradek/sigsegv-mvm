#include "util/pooled_string.h"
#include "util/misc.h"
#include "link/link.h"
#include "utlhashtable.h"
#include "mod.h"

#ifdef SE_TF2
class CGameStringPool : public CBaseGameSystem
{
public:
    CUtlRBTree<const char *, unsigned short> pool;
    CUtlHashtable<const void*, const char*> m_KeyLookupCache;
};
GlobalThunk<CGameStringPool> g_GameStringPool("g_GameStringPool");
#endif

namespace Util::PooledStringMod
{
    DETOUR_DECL_MEMBER(void, CGameStringPool_LevelShutdownPostEntity)
    {
        DETOUR_MEMBER_CALL();
        for (auto &string : PooledString::List()) {
            string->Reset();
        }
    }


    class CMod : public IMod
    {
    public:
        CMod() : IMod("Util:Pooled_String")
        {
            MOD_ADD_DETOUR_MEMBER(CGameStringPool_LevelShutdownPostEntity, "CGameStringPool::LevelShutdownPostEntity");
        }
        virtual bool OnLoad() override
        {
            for (auto &string : PooledString::List()) {
                string->Reset();
            }
            return true;
        }

        virtual bool EnableByDefault() override { return true; }
    };
    CMod s_Mod;

#ifdef SE_TF2
    // Reimplement pooled strings to circumevent the 65k string limit
    // a pointer, to not deconstruct automatically when the extension is unloaded;
    std::unordered_map<std::string, const char *, CaseInsensitiveHash, CaseInsensitiveCompare> *pool;
    CUtlHashtable<const void*, const char*> pool_fast2;
    
    inline string_t AllocPooledString_StaticConstantStringPointerDo(const char *str)
    {
        const char * &cached = pool_fast2[ pool_fast2.Insert( str, NULL ) ];
        if (cached == nullptr) {
			auto [it, inserted] = pool->emplace(str, str);
            if (inserted) {
                it->second = it->first.c_str();
            }
            cached = it->second;
		}
        return MAKE_STRING(cached);
    }
    
    inline string_t AllocPooledStringDo(const char *str)
    {
        if (str == nullptr || str[0] == '\0') return NULL_STRING;

        auto [it, inserted] = pool->emplace(str, str);
        if (inserted) {
            it->second = it->first.c_str();
        }
        return MAKE_STRING(it->second);
    }
    
    DETOUR_DECL_STATIC(string_t, AllocPooledString_StaticConstantStringPointer, const char *str)
    {
        return AllocPooledString_StaticConstantStringPointerDo(str);
    }

    DETOUR_DECL_STATIC(string_t, AllocPooledString, const char *str)
    {
        return AllocPooledStringDo(str);
    }

    DETOUR_DECL_STATIC(string_t, FindPooledString, const char *str)
    {
        auto it = pool->find(str);
        return MAKE_STRING(it != pool->end() ? it->second : nullptr);
    }


    DETOUR_DECL_MEMBER(void, CGameStringPool_LevelShutdownPostEntity2)
    {
        DETOUR_MEMBER_CALL();
        pool->clear();
        pool_fast2.RemoveAll();
    }

    class CModFix : public IMod
    {
    public:
        CModFix() : IMod("Util:Pooled_String_Fix")
        {
            MOD_ADD_DETOUR_MEMBER(CGameStringPool_LevelShutdownPostEntity2, "CGameStringPool::LevelShutdownPostEntity");
            MOD_ADD_DETOUR_STATIC(AllocPooledString, "AllocPooledString");
            MOD_ADD_DETOUR_STATIC(AllocPooledString_StaticConstantStringPointer, "AllocPooledString_StaticConstantStringPointer");
            MOD_ADD_DETOUR_STATIC(FindPooledString, "FindPooledString");
            pool = new std::unordered_map<std::string, const char *, CaseInsensitiveHash, CaseInsensitiveCompare>;
        }

        virtual void OnEnable() override
        {
            auto &gamePool = g_GameStringPool.GetRef().pool;
            for ( int i = gamePool.FirstInorder(); i != gamePool.InvalidIndex(); i = gamePool.NextInorder( i ) )
            {
                auto str = gamePool[i];
                (*pool)[str] = str;
            }
        }
        virtual void OnDisable() override
        {
            for (auto &entry : *pool) {
                g_GameStringPool.GetRef().pool.InsertIfNotFound(entry.second);
            }
        }

        virtual bool EnableByDefault() override { return false; }
    };
    CModFix s_ModFix;

	ConVar sig_util_pooled_string_fix_always("sig_util_pooled_string_fix", "0", FCVAR_NOTIFY,
		"Mod: Always enable the 65k+ pooled strings fix",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
            if (static_cast<ConVar *>(pConVar)->GetBool()) {
			    s_ModFix.Toggle(true);
            }
		});

    inline void EnableIfOverStringLimit() {
        if (s_ModFix.IsEnabled()) return;
        
        if (g_GameStringPool.GetRef().pool.Count() > 60000) {
            s_ModFix.Enable();
        }
    }
}
#endif

static StaticFuncThunk<string_t, const char *> ft_AllocPooledString("AllocPooledString");
string_t AllocPooledString(const char *pszValue) 
{
#ifdef SE_TF2
    if (!Util::PooledStringMod::s_ModFix.IsEnabled()) {
        if (g_GameStringPool.GetRef().pool.Count() > 60000) {
            Util::PooledStringMod::s_ModFix.Enable();
        }
        return ft_AllocPooledString(pszValue);
    }

    return Util::PooledStringMod::AllocPooledStringDo(pszValue);
#else 
    return ft_AllocPooledString(pszValue); 
#endif
}

static StaticFuncThunk<string_t, const char *> ft_AllocPooledString_StaticConstantStringPointer("AllocPooledString_StaticConstantStringPointer");
string_t AllocPooledString_StaticConstantStringPointer(const char *pszGlobalConstValue) { 
#ifdef SE_TF2
    if (Util::PooledStringMod::s_ModFix.IsEnabled()) {
        return Util::PooledStringMod::AllocPooledString_StaticConstantStringPointerDo(pszGlobalConstValue);
    }
    else {
        return ft_AllocPooledString_StaticConstantStringPointer(pszGlobalConstValue);
    }
#else

    return ft_AllocPooledString_StaticConstantStringPointer(pszGlobalConstValue); 
#endif
}

static StaticFuncThunk<string_t, const char *> ft_FindPooledString("FindPooledString");
string_t FindPooledString(const char *pszValue) { return ft_FindPooledString(pszValue); }