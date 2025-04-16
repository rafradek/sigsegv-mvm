#include "mod.h"
#include "stub/populators.h"
#include "util/scope.h"


// this mod is totally wrong.


namespace Mod::MvM::Reset_After_Victory
{
	RefCount rc_CPopulationManager_Update;
	DETOUR_DECL_MEMBER(void, CPopulationManager_Update)
	{
		SCOPED_INCREMENT(rc_CPopulationManager_Update);
		DETOUR_MEMBER_CALL(CPopulationManager_Update)();
	}
	
	DETOUR_DECL_MEMBER(void, CPopulationManager_CycleMission)
	{
		auto popmgr = reinterpret_cast<CPopulationManager *>(this);
		
		// when asked to cycle the mission by the mission-victory code, just reset the map instead
		if (rc_CPopulationManager_Update > 0) {
			DevMsg("[MvM:Reset_After_Victory] Intercepted call to CPopulationManager::CycleMission.\n");
			DevMsg("[MvM:Reset_After_Victory] Calling CPopulationManager::ResetMap instead.\n");
			popmgr->ResetMap();
			return;
		}
		
		// if asked by tf_mvm_nextmission etc, just do the normal stuff
		DETOUR_MEMBER_CALL(CPopulationManager_CycleMission)();
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:Reset_After_Victory")
		{
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_Update,       "CPopulationManager::Update");
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_CycleMission, "CPopulationManager::CycleMission");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_reset_after_victory", "0", FCVAR_NOTIFY,
		"Mod: possibly work around post-victory-countdown mission-cycling crash by just forcing a mission reset instead",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
