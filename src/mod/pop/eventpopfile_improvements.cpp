#include "mod.h"
#include "stub/tfbot.h"
#include "stub/tf_objective_resource.h"
#include "stub/populators.h"
#include "stub/tf_shareddefs.h"
#include "util/scope.h"


namespace Mod::Pop::EventPopfile_Improvements
{
	RefCount rc_CMissionPopulator_UpdateMissionDestroySentries;
	DETOUR_DECL_MEMBER(int, CMissionPopulator_UpdateMissionDestroySentries, const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents)
	{
		SCOPED_INCREMENT(rc_CMissionPopulator_UpdateMissionDestroySentries);
		return DETOUR_MEMBER_CALL(where, ents);
	}
	
	DETOUR_DECL_MEMBER(void, CTFBot_AddItem, const char *item)
	{
		if (rc_CMissionPopulator_UpdateMissionDestroySentries > 0 && strncmp(item, "Zombie ", strlen("Zombie ")) == 0) {
				return;
		}
		
		DETOUR_MEMBER_CALL(item);
	}
	
	
	DETOUR_DECL_MEMBER(void, CPopulationManager_UpdateObjectiveResource)
	{
		DETOUR_MEMBER_CALL();
		
		// ConVarRef tf_forced_holiday("tf_forced_holiday");
		// if (TFObjectiveResource()->m_nMvMEventPopfileType != 0u) {
		// 	tf_forced_holiday.SetValue(kHoliday_Halloween);
		// } else {
		// 	tf_forced_holiday.SetValue(kHoliday_None);
		// }
	}
	
	
	DETOUR_DECL_STATIC(bool, UTIL_IsHolidayActive, int holiday)
	{
		if (holiday == kHoliday_HalloweenOrFullMoonOrValentines && rc_CMissionPopulator_UpdateMissionDestroySentries > 0) {
			return true;
		}
		
		return DETOUR_STATIC_CALL(holiday);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Pop:EventPopfile_Improvements")
		{
			MOD_ADD_DETOUR_MEMBER(CMissionPopulator_UpdateMissionDestroySentries, "CMissionPopulator::UpdateMissionDestroySentries");
			MOD_ADD_DETOUR_MEMBER(CTFBot_AddItem,      "CTFBot::AddItem");
			
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_UpdateObjectiveResource, "CPopulationManager::UpdateObjectiveResource");
			
			MOD_ADD_DETOUR_STATIC(UTIL_IsHolidayActive, "UTIL_IsHolidayActive");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_pop_eventpopfile_improvements", "0", FCVAR_NOTIFY,
		"Mod: make EventPopfile work regardless of holiday; set tf_forced_holiday based on EventPopfile; disable zombie souls for sentry busters",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
