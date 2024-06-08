#include "mod.h"
#include "stub/tfweaponbase.h"
#include "util/scope.h"


namespace Mod::Attr::HealOnKill_Overheal_Melee
{
	RefCount rc_MeleeHoK;
	DETOUR_DECL_MEMBER(void, CTFPlayer_OnKilledOther_Effects, CBaseEntity *other, const CTakeDamageInfo& info)
	{
		auto melee = rtti_cast<CTFWeaponBaseMelee *>(info.GetWeapon());
		if (melee != nullptr) ++rc_MeleeHoK;
		
		DETOUR_MEMBER_CALL(other, info);
		
		if (melee != nullptr) --rc_MeleeHoK;
	}
	
	DETOUR_DECL_MEMBER(int, CTFPlayer_TakeHealth, float flHealth, int bitsDamageType)
	{
		if (rc_MeleeHoK > 0) {
			bitsDamageType |= DMG_IGNORE_MAXHEALTH;
		}
		
		return DETOUR_MEMBER_CALL(flHealth, bitsDamageType);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Attr:HealOnKill_Overheal_Melee")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_OnKilledOther_Effects, "CTFPlayer::OnKilledOther_Effects");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_TakeHealth,            "CTFPlayer::TakeHealth");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_attr_healonkill_overheal_melee", "0", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Mod: re-enable overheal from the \"heal_on_kill\" attribute for melee weapons",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
