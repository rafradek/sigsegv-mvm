#include "mod.h"
#include "stub/baseentity.h"


namespace Mod::Etc::Debug_Mission_Change_Crash
{
	// same as CGlobalEntityList::IsEntityPtr
	bool IsEntityPtr(void *ptr)
	{
		if (ptr == nullptr) return false;
		
		for (const CEntInfo *info = g_pEntityList->FirstEntInfo(); info != nullptr; info = info->m_pNext) {
			if (reinterpret_cast<void *>(info->m_pEntity) == ptr) {
				return true;
			}
		}
		
		return false;
	}
	
	// like CBaseEntityList::GetEntInfoIndex
	int GetEntityIndexOfPtr(void *ptr)
	{
		if (ptr == nullptr) return -1;
		
		for (const CEntInfo *info = g_pEntityList->FirstEntInfo(); info != nullptr; info = info->m_pNext) {
			if (reinterpret_cast<void *>(info->m_pEntity) == ptr) {
				return (info - g_pEntityList->GetEntInfoPtrByIndex(0));
			}
		}
		
		return -1;
	}
	
	
	DETOUR_DECL_MEMBER(void, CBaseEntity_SetNextThink_index, int nContextIndex, float thinkTime)
	{
		auto ent = reinterpret_cast<CBaseEntity *>(this);
		
		if (ent == nullptr) {
			BACKTRACE();
			Msg("[CBaseEntity::SetNextThink(index)] ENTER: this == nullptr (ctx %d, next @ %.3f, reltime %.3f)\n", nContextIndex, thinkTime, thinkTime - gpGlobals->curtime);
		} else if (!IsEntityPtr(ent)) {
			BACKTRACE();
			Msg("[CBaseEntity::SetNextThink(index)] ENTER: !IsEntityPtr(this) (ent @ 0x%08x, ctx %d, next @ %.3f, reltime %.3f)\n", (uintptr_t)ent, nContextIndex, thinkTime, thinkTime - gpGlobals->curtime);
		} else if (*reinterpret_cast<void **>(ent) == nullptr) {
			BACKTRACE();
			Msg("[CBaseEntity::SetNextThink(index)] ENTER: vtptr == nullptr (ent @ 0x%08x, ent #%d, ctx %d, next @ %.3f, reltime %.3f)\n", (uintptr_t)ent, GetEntityIndexOfPtr(ent), nContextIndex, thinkTime, thinkTime - gpGlobals->curtime);
		}
		
		DETOUR_MEMBER_CALL(nContextIndex, thinkTime);
		
		if (ent == nullptr || *reinterpret_cast<void **>(ent) == nullptr) {
			BACKTRACE();
			Msg("[CBaseEntity::SetNextThink(index)] LEAVE\n");
		}
	}
	
	DETOUR_DECL_MEMBER(void, CBaseEntity_SetNextThink_name, float nextThinkTime, const char *szContext)
	{
		auto ent = reinterpret_cast<CBaseEntity *>(this);
		
		if (ent == nullptr) {
			Msg("[CBaseEntity::SetNextThink(name)] ENTER: this == nullptr (ctx \"%s\", next @ %.3f, reltime %.3f)\n", szContext, nextThinkTime, nextThinkTime - gpGlobals->curtime);
			BACKTRACE();
		} else if (!IsEntityPtr(ent)) {
			Msg("[CBaseEntity::SetNextThink(name)] ENTER: !IsEntityPtr(this) (ent @ 0x%08x, ctx \"%s\", next @ %.3f, reltime %.3f)\n", (uintptr_t)ent, szContext, nextThinkTime, nextThinkTime - gpGlobals->curtime);
			BACKTRACE();
		} else if (*reinterpret_cast<void **>(ent) == nullptr) {
			Msg("[CBaseEntity::SetNextThink(name)] ENTER: vtptr == nullptr (ent @ 0x%08x, ent #%d, ctx \"%s\", next @ %.3f, reltime %.3f)\n", (uintptr_t)ent, GetEntityIndexOfPtr(ent), szContext, nextThinkTime, nextThinkTime - gpGlobals->curtime);
			BACKTRACE();
		}
		
		DETOUR_MEMBER_CALL(nextThinkTime, szContext);
		
		if (ent == nullptr || *reinterpret_cast<void **>(ent) == nullptr) {
			BACKTRACE();
			Msg("[CBaseEntity::SetNextThink(name)] LEAVE\n");
		}
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Debug_Mission_Change_Crash")
		{
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_SetNextThink_index, "CBaseEntity::SetNextThink [index]");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_SetNextThink_name,  "CBaseEntity::SetNextThink [name]");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_etc_debug_mission_change_crash", "0", FCVAR_NOTIFY,
		"Mod: debug crash that occurs in CBaseEntity::SetNextThink when an MvM mission ends and the next one starts",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
