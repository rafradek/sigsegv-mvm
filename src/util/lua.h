#ifndef _INCLUDE_SIGSEGV_UTIL_LUA_H_
#define _INCLUDE_SIGSEGV_UTIL_LUA_H_

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

namespace Util::Lua
{
    struct GlobalCallback
    {
        std::string m_Name;
        int m_iRefFunc;
    };
    class LuaEntityModule;

    class LuaTimer
    {
    public:
        LuaTimer(int id, float delay, int repeats, int reffunc, int refparam) : m_iID(id), m_flNextCallTime(gpGlobals->curtime + delay), m_flDelay(delay), m_iRepeats(repeats), m_iRefFunc(reffunc), m_iRefParam(refparam) {}

        void Destroy(lua_State *l);

        int m_iID;
        float m_flNextCallTime;
        float m_flDelay;
        int m_iRepeats;
        int m_iRefFunc;
        int m_iRefParam;
        bool m_bDestroyed = false;
    };

    class LuaState : public AutoList<LuaState>
    {
    public:
        LuaState();

        virtual ~LuaState();

        lua_State *GetState() {
            return l;
        }

        void SwitchState();

        void DoString(const char *str, bool execute);

        void DoFile(const char *str, bool execute);

        void CallGlobal(const char *str, int numargs);

        bool CheckGlobal(const char *str);

        int Call(int numargs, int numret);

        bool CallGlobalCallback(const char *name, int numargs, int numret);

        void UpdateTimers();

        virtual void TimerAdded() {}
        virtual void AllTimersRemoved() {}

        int AddTimer(float delay, int repeats, int reffunc, int refparam);
        bool StopTimer(int id);

        bool HasTimers() { return !timers.empty(); }

        void EntityDeleted(CBaseEntity *entity);
        void EntityCallbackAdded(CBaseEntity *entity);

        void Activate();

        void AddConVarValue(ConVar *convar);

        const std::list<LuaTimer> &GetTimers() { return timers; }
        const std::unordered_set<CBaseEntity *> &GetCallbackEntities() { return callbackEntities; }
        const std::unordered_map<ConVar *, std::string> &GetOverriddenConvars() { return convarValueToRestore; }
        size_t GetMemoryUsageBytes() { return m_iMemoryUsage; }
        void AddGlobalCallback(const char *name, int reffunc) { globalCallbacks.push_back({name, reffunc}); };
        void RemoveGlobalCallback(int reffunc);

        CEconItemAttributeDefinition *GetAttributeDefinitionByNameCached(const char *name);

    private:
        std::list<LuaTimer>::iterator DestroyTimer(std::list<LuaTimer>::iterator it);
        lua_State *l;

        int m_iVectorMeta = LUA_NOREF;
        const void *m_pVectorMeta = nullptr;
        int m_iEntityMeta = LUA_NOREF;
        const void *m_pEntityMeta = nullptr;
        int m_iEntityTableStorage = LUA_NOREF;
        int m_iPropMeta = LUA_NOREF;
        int m_iEventMeta = LUA_NOREF;
        int m_iSavedataMeta = LUA_NOREF;

        bool m_bTimerLoop = false;

        std::list<LuaTimer> timers;
        int m_iNextTimerID = 0;
        std::unordered_set<CBaseEntity *> callbackEntities;
        std::unordered_map<ConVar *, std::string> convarValueToRestore; 
        std::list<GlobalCallback> globalCallbacks;
        size_t m_iMemoryUsage = 0;

        std::unordered_map<std::string, CEconItemAttributeDefinition *> attributeIdCache;
    };

    void LFromVariant(lua_State *l, variant_t &variant);
    const EHANDLE *LEntityAlloc(lua_State *l, CBaseEntity *entity);
    Vector *LVectorGetNoCheckNoInline(lua_State *l, int index);
    
    bool DoCollideTestInternal(CBaseEntity *entity1, CBaseEntity *entity2, LuaEntityModule *mod, bool &result);
}
#endif