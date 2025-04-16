#include "mod.h"
#include "stub/baseentity.h"
#include "stub/populators.h"
#include "util/scope.h"


namespace Mod::MvM::PopMgr_Think_Fix
{
	RefCount rc_CPopulationManager_ctor;
	DETOUR_DECL_MEMBER(void, CPopulationManager_ctor)
	{
		Msg("ENTER CPopulationManager ctor\n");
		SCOPED_INCREMENT(rc_CPopulationManager_ctor);
		DETOUR_MEMBER_CALL(CPopulationManager_ctor)();
		Msg("LEAVE CPopulationManager ctor\n");
	}
	
	DETOUR_DECL_MEMBER(void, CBaseEntity_SetNextThink_name, float nextThinkTime, const char *szContext)
	{
		if (rc_CPopulationManager_ctor > 0) {
			Msg("IGNORE SetNextThink from CPopulationManager ctor\n");
			return;
		}
		
		DETOUR_MEMBER_CALL(CBaseEntity_SetNextThink_name)(nextThinkTime, szContext);
	}
	
	DETOUR_DECL_MEMBER(BASEPTR, CBaseEntity_ThinkSet, BASEPTR func, float thinkTime, const char *szContext)
	{
		if (rc_CPopulationManager_ctor > 0) {
			Msg("IGNORE ThinkSet from CPopulationManager ctor\n");
			return nullptr;
		}
		
		return DETOUR_MEMBER_CALL(CBaseEntity_ThinkSet)(func, thinkTime, szContext);
	}
	
	
	RefCount rc_UTIL_PrecacheOther;
	DETOUR_DECL_STATIC(void, UTIL_PrecacheOther, const char *szClassname, const char *s2)
	{
		SCOPED_INCREMENT_IF(rc_UTIL_PrecacheOther, szClassname != nullptr && strcmp(szClassname, "info_populator") == 0);
		DETOUR_STATIC_CALL(UTIL_PrecacheOther)(szClassname, s2);
	}
	
	DETOUR_DECL_STATIC(CBaseEntity *, CreateEntityByName, const char *className, int iForceEdictIndex)
	{
		CBaseEntity *ent = DETOUR_STATIC_CALL(CreateEntityByName)(className, iForceEdictIndex);
		
		if (rc_UTIL_PrecacheOther <= 0 && ent != nullptr && className != nullptr && strcmp(className, "info_populator") == 0) {
			Msg("CALL DispatchSpawn in CreateEntityByName(\"info_populator\")\n");
			DispatchSpawn(ent);
		}
		
		return ent;
	}
	
	
	DETOUR_DECL_MEMBER(void, CPopulationManager_Spawn)
	{
		auto popmgr = reinterpret_cast<CPopulationManager *>(this);
		
		Msg("ENTER CPopulationManager::Spawn\n");
		
		static auto ptr_CPopulationManager_Update = MakePtrToMemberFunc<CPopulationManager, void>(AddrManager::GetAddr("CPopulationManager::Update"));
		
		Msg("CALL ThinkSet\n");
		popmgr->ThinkSet(ptr_CPopulationManager_Update);
		Msg("CALL SetNextThink\n");
		popmgr->SetNextThink(gpGlobals->curtime);
		
		DETOUR_MEMBER_CALL(CPopulationManager_Spawn)();
		
		Msg("LEAVE CPopulationManager::Spawn\n");
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:PopMgr_Think_Fix")
		{
			// ignore calls to ThinkSet and SetNextThink from CPopulationManager's ctor
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_ctor,        "CPopulationManager::CPopulationManager [C1]");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_SetNextThink_name,  "CBaseEntity::SetNextThink [name]");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_ThinkSet,           "CBaseEntity::ThinkSet");
			
			// ensure that when CPopulationManager entities are created for real, that DispatchSpawn is called
			MOD_ADD_DETOUR_STATIC(UTIL_PrecacheOther, "UTIL_PrecacheOther");
			MOD_ADD_DETOUR_STATIC(CreateEntityByName, "CreateEntityByName");
			
			// re-add the calls to ThinkSet and SetNextThink in CPopulationManager::Spawn
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_Spawn, "CPopulationManager::Spawn");
		}
		
		virtual bool EnableByDefault() override { return true; }
	};
	CMod s_Mod;
	
	
	// arguably this should be initialized to "1"...?
	ConVar cvar_enable("sig_mvm_popmgr_think_fix", "0", FCVAR_NOTIFY,
		"Mod: move CPopulationManager's ThinkSet/SetNextThink calls into Spawn and ensure DispatchSpawn happens",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
