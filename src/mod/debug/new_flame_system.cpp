#include "mod.h"
#include "stub/point.h"


namespace Mod::Debug::New_Flame_System
{
	// #1: Check how many times per tick the following functions are called:
	// - CTFPointManager::PointThink
	// - CTFPointManager::Update
	// - CTFFlameManager::Update
	// - CTFFlameManager::PostEntityThink
	// FINDINGS: all called once per tick; PointThink happens every tick.
	
	
	DETOUR_DECL_MEMBER(void, CTFPointManager_PointThink)
	{
		Msg("[tick %d] CTFPointManager::PointThink %08x\n", gpGlobals->tickcount, (uintptr_t)this);
		DETOUR_MEMBER_CALL(CTFPointManager_PointThink)();
	}
	
	DETOUR_DECL_MEMBER(void, CTFPointManager_Update)
	{
		Msg("[tick %d] CTFPointManager::Update %08x\n", gpGlobals->tickcount, (uintptr_t)this);
		DETOUR_MEMBER_CALL(CTFPointManager_Update)();
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFFlameManager_PostEntityThink)
	{
		Msg("[tick %d] CTFFlameManager::PostEntityThink %08x\n", gpGlobals->tickcount, (uintptr_t)this);
		DETOUR_MEMBER_CALL(CTFFlameManager_PostEntityThink)();
	}
	
	DETOUR_DECL_MEMBER(void, CTFFlameManager_Update)
	{
		Msg("[tick %d] CTFFlameManager::Update %08x\n", gpGlobals->tickcount, (uintptr_t)this);
		DETOUR_MEMBER_CALL(CTFFlameManager_Update)();
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:New_Flame_System")
		{
		//	MOD_ADD_DETOUR_MEMBER(CTFPointManager_PointThink, "CTFPointManager::PointThink");
		//	MOD_ADD_DETOUR_MEMBER(CTFPointManager_Update,     "CTFPointManager::Update");
			
		//	MOD_ADD_DETOUR_MEMBER(CTFFlameManager_PostEntityThink, "CTFFlameManager::PostEntityThink");
		//	MOD_ADD_DETOUR_MEMBER(CTFFlameManager_Update,          "CTFFlameManager::Update");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_new_flame_system", "0", FCVAR_NOTIFY,
		"Debug: figure out all the problems with the new flame system introduced in Jungle Inferno",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
