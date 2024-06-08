#include "mod.h"
#include "stub/projectiles.h"
#include "stub/tf_shareddefs.h"
#include "util/scope.h"


namespace Mod::Etc::Loose_Cannon_Damage_Fix
{
	RefCount rc_CTFGrenadePipebombProjectile_PipebombTouch;
	CTFGrenadePipebombProjectile *pGrenade = nullptr;
	DETOUR_DECL_MEMBER(void, CTFGrenadePipebombProjectile_PipebombTouch, CBaseEntity *pOther)
	{
		SCOPED_INCREMENT(rc_CTFGrenadePipebombProjectile_PipebombTouch);
		pGrenade = reinterpret_cast<CTFGrenadePipebombProjectile *>(this);
		DETOUR_MEMBER_CALL(pOther);
		pGrenade = nullptr;
	}
	
	DETOUR_DECL_MEMBER(int, CBaseEntity_TakeDamage, const CTakeDamageInfo& info)
	{
		auto ent = reinterpret_cast<CBaseEntity *>(this);
		
		if (rc_CTFGrenadePipebombProjectile_PipebombTouch > 0 && info.GetDamageCustom() == TF_DMG_CUSTOM_CANNONBALL_PUSH && ent != nullptr && pGrenade != nullptr) {
			CBaseEntity *thrower = info.GetAttacker();
			if (thrower != nullptr) {
				float flDamage = pGrenade->GetDamage();
				
				/* thanks, devs, for never updating the 0.6f multiplier in CTFGrenadePipebombProjectile::Create */
				flDamage *= (0.5f / 0.6f);
				
				float flDistSqr = thrower->GetAbsOrigin().DistToSqr(ent->GetAbsOrigin());
				constexpr float flDistSqrMin = 1.0f * 512.0f * 512.0f;
				constexpr float flDistSqrMax = 3.0f * 512.0f * 512.0f;
				
				flDamage = RemapValClamped(flDistSqr, flDistSqrMin, flDistSqrMax, flDamage, flDamage / 2.0f);
				
				/* const: "nope" */
				const_cast<CTakeDamageInfo&>(info).SetDamage(flDamage);
				
			//	DevMsg("distsqr: %.0f | %.0f | %.0f\n", flDistSqrMin, flDistSqr, flDistSqrMax);
			//	DevMsg("flDamage = %.1f\n", flDamage);
			}
		}
		
		return DETOUR_MEMBER_CALL(info);
	}

//	DETOUR_DECL_MEMBER(void, CBaseGrenade_SetDamage, float flDamage)
//	{
//		auto grenade = reinterpret_cast<CBaseGrenade *>(this);
//		
//		float flBefore = grenade->GetDamage();
//		DETOUR_MEMBER_CALL(flDamage);
//		float flAfter = grenade->GetDamage();
//		
//		DevMsg("CBaseGrenade::SetDamage: [%.1f --> %.1f]\n", flBefore, flAfter);
//	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Loose_Cannon_Damage_Fix")
		{
			MOD_ADD_DETOUR_MEMBER(CTFGrenadePipebombProjectile_PipebombTouch, "CTFGrenadePipebombProjectile::PipebombTouch");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_TakeDamage,                     "CBaseEntity::TakeDamage");
			
		//	MOD_ADD_DETOUR_MEMBER(CBaseGrenade_SetDamage, "CBaseGrenade::SetDamage");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_etc_loose_cannon_damage_fix", "0", FCVAR_NOTIFY,
		"Mod: fix the Tough Break update change that made Loose Cannon projectile impact damage unaffected by multipliers",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
