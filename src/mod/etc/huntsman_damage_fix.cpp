#include "mod.h"
#include "stub/tfweaponbase.h"
#include "util/scope.h"


namespace Mod::Etc::Huntsman_Damage_Fix
{
	RefCount rc_CTFCompoundBow_GetProjectileDamage;
	float flBaseClassProjectileDamage;
	DETOUR_DECL_MEMBER(float, CTFCompoundBow_GetProjectileDamage)
	{
		auto bow = reinterpret_cast<CTFCompoundBow *>(this);
		
		if (!ToTFPlayer(bow->GetOwner())->IsBot())
			return DETOUR_MEMBER_CALL();

		float flChargeDamage;
		
		{
			flBaseClassProjectileDamage = 0.0f;
			SCOPED_INCREMENT(rc_CTFCompoundBow_GetProjectileDamage);
			
			(void)DETOUR_MEMBER_CALL();
			
			flChargeDamage = flBaseClassProjectileDamage;
		}
		
		return 50 + flChargeDamage;
	}
	
	DETOUR_DECL_MEMBER(float, CTFWeaponBaseGun_GetProjectileDamage)
	{
		float flDamage = DETOUR_MEMBER_CALL();
		
		if (rc_CTFCompoundBow_GetProjectileDamage > 0) {
			flBaseClassProjectileDamage = flDamage;
		}
		
		return flDamage;
	}
	
	ConVar cvar_huntsman_charge_time("sig_huntsman_charge_time_upgrade_ratio", "1", FCVAR_NOTIFY,
		"Charge time upgrade ratio");
	DETOUR_DECL_MEMBER(float, CTFCompoundBow_GetChargeMaxTime)
	{
		float normal_time = 1.0f;

		float actual_charge = DETOUR_MEMBER_CALL();

		auto bow = reinterpret_cast<CTFCompoundBow *>(this);
		
		if (ToTFPlayer(bow->GetOwner())->IsBot())
			return actual_charge;

		return RemapVal( cvar_huntsman_charge_time.GetFloat(), 0, 1, normal_time, actual_charge );
	}

	class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Huntsman_Damage_Fix")
		{
			MOD_ADD_DETOUR_MEMBER(CTFCompoundBow_GetProjectileDamage,   "CTFCompoundBow::GetProjectileDamage");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_GetProjectileDamage, "CTFWeaponBaseGun::GetProjectileDamage");
			MOD_ADD_DETOUR_MEMBER(CTFCompoundBow_GetChargeMaxTime, "CTFCompoundBow::GetChargeMaxTime");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_etc_huntsman_damage_fix", "0", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Mod: restore pre fix bot huntsman damage",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
