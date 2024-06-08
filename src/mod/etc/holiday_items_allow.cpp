#include "mod.h"
#include "stub/tf_shareddefs.h"
#include "stub/econ.h"
#include "util/misc.h"


namespace Mod::Etc::Holiday_Items_Allow
{
	/* the only actual callers of this function are econ-related */
	RefCount rc_CTFPlayer_ItemIsAllowed;
	DETOUR_DECL_MEMBER(bool, CTFPlayer_ItemIsAllowed, CEconItemView *item_view)
	{
		SCOPED_INCREMENT_IF(rc_CTFPlayer_ItemIsAllowed, item_view != nullptr && item_view->GetStaticData() != nullptr && item_view->GetStaticData()->GetKeyValues() != nullptr && FStrEq(item_view->GetStaticData()->GetKeyValues()->GetString("equip_region"), "zombie_body"));
		return DETOUR_MEMBER_CALL(item_view);
	}

	DETOUR_DECL_MEMBER(bool, CEconItemDefinition_BInitFromKV, KeyValues *kv, CUtlVector<CUtlString> *errors)
	{
		Msg("Has item\n");
		auto holiday = kv->FindKey("holiday_restriction");
		if (holiday != nullptr && !FStrEq(kv->GetString("equip_region"), "zombie_body")) {
			
			kv->RemoveSubKey(holiday);
			holiday->deleteThis();
		}
		return DETOUR_MEMBER_CALL(kv, errors);
	}

	DETOUR_DECL_STATIC(int, UTIL_GetHolidayForString, const char *str)
	{
		return rc_CTFPlayer_ItemIsAllowed ? DETOUR_STATIC_CALL(str) : kHoliday_None;
	}
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Holiday_Items_Allow")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ItemIsAllowed, "CTFPlayer::ItemIsAllowed");
			//MOD_ADD_DETOUR_MEMBER(CEconItemDefinition_BInitFromKV,  "CEconItemDefinition::BInitFromKV");
			MOD_ADD_DETOUR_STATIC(UTIL_GetHolidayForString, "UTIL_GetHolidayForString");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_etc_holiday_items_allow", "0", FCVAR_NOTIFY,
		"Mod: allow holiday-restricted loadout items regardless of the holiday state",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
