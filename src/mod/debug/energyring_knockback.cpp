#include "mod.h"
#include "stub/baseentity.h"


namespace Mod::Debug::EnergyRing_Knockback
{
	// g_aWeaponDamageTypes
	// TF_WEAPON_RAYGUN:     0x00220002
	// TF_WEAPON_DRG_POMSON: 0x00220002
	
	// 0x00220002 = DMG_BULLET | DMG_NOCLOSEDISTANCEMOD | DMG_NODISTANCEMOD
	
	
	ConVar cvar_prevent("sig_debug_energyring_knockback_prevent", "0", FCVAR_NOTIFY,
		"Debug: add DMG_PREVENT_PHYSICS_FORCE to energy ring damage type");
	
	
	DETOUR_DECL_MEMBER(int, CTFGameRules_ApplyOnDamageModifyRules, CTakeDamageInfo& info, CBaseEntity *pVictim, bool b1)
	{
		if (cvar_prevent.GetBool()) {
			CBaseEntity *weapon = info.GetWeapon();
			if (weapon != nullptr && (weapon->ClassMatches("tf_weapon_raygun") || weapon->ClassMatches("tf_weapon_drg_pomson"))) {
				DevMsg("CTakeDamageInfo from '%s'; adding DMG_PREVENT_PHYSICS_FORCE.\n", weapon->GetClassname());
				DevMsg("- before: %08x\n", info.GetDamageType());
				info.AddDamageType(DMG_PREVENT_PHYSICS_FORCE);
				DevMsg("- after:  %08x\n", info.GetDamageType());
			}
		}
		
		return DETOUR_MEMBER_CALL(info, pVictim, b1);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:EnergyRing_Knockback")
		{
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_ApplyOnDamageModifyRules, "CTFGameRules::ApplyOnDamageModifyRules");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_energyring_knockback", "0", FCVAR_NOTIFY,
		"Debug: figure out why Pomson/Bison projectiles are inflicting knockback",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
