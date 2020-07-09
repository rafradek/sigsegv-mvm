#include "mod.h"
#include "stub/tfbot.h"
#include "util/scope.h"
#include "stub/projectiles.h"

namespace Mod::Bot::MultiClass_Weapon
{
	/* a less strict version of TranslateWeaponEntForClass (don't return empty strings) */

	
	
	
	void SetCustomProjectileModel(CTFWeaponBaseGun *weapon, CBaseAnimating *proj){
		CAttributeList &attrlist = weapon->GetItem()->GetAttributeList();
		auto attr = attrlist.GetAttributeByName("custom projectile model");
		if (attr != nullptr) {
			char modelname[255];
			GetItemSchema()->GetAttributeDefinitionByName("custom projectile model")->ConvertValueToString(*(attr->GetValuePtr()),modelname,255);
			proj->SetModel(modelname);
			DevMsg("Setting custom projectile model to %s \n",modelname);
		}
	}
	
	class CTFWeaponInfo;
	DETOUR_DECL_MEMBER(CBaseAnimating *, CTFJar_CreateJarProjectile, Vector const& vec, QAngle const& ang, Vector const& vec2, Vector const& vec3, CBaseCombatCharacter* thrower, CTFWeaponInfo const& info)
	{
		auto proj = DETOUR_MEMBER_CALL(CTFJar_CreateJarProjectile)(vec, ang, vec2, vec3, thrower, info);
		if (proj != nullptr) {
			SetCustomProjectileModel(reinterpret_cast<CTFWeaponBaseGun *>(this),proj);
		}
		return proj;
	}
	DETOUR_DECL_MEMBER(CBaseAnimating *, CTFJarMilk_CreateJarProjectile, Vector const& vec, QAngle const& ang, Vector const& vec2, Vector const& vec3, CBaseCombatCharacter* thrower, CTFWeaponInfo const& info)
	{
		auto proj = DETOUR_MEMBER_CALL(CTFJarMilk_CreateJarProjectile)(vec, ang, vec2, vec3, thrower, info);
		if (proj != nullptr) {
			SetCustomProjectileModel(reinterpret_cast<CTFWeaponBaseGun *>(this),proj);
		}
		return proj;
	}
	DETOUR_DECL_MEMBER(CBaseAnimating *, CTFJarGas_CreateJarProjectile, Vector const& vec, QAngle const& ang, Vector const& vec2, Vector const& vec3, CBaseCombatCharacter* thrower, CTFWeaponInfo const& info)
	{
		auto proj = DETOUR_MEMBER_CALL(CTFJarGas_CreateJarProjectile)(vec, ang, vec2, vec3, thrower, info);
		if (proj != nullptr) {
			SetCustomProjectileModel(reinterpret_cast<CTFWeaponBaseGun *>(this),proj);
		}
		return proj;
	}
	DETOUR_DECL_MEMBER(CBaseAnimating *, CTFCleaver_CreateJarProjectile, Vector const& vec, QAngle const& ang, Vector const& vec2, Vector const& vec3, CBaseCombatCharacter* thrower, CTFWeaponInfo const& info)
	{
		auto proj = DETOUR_MEMBER_CALL(CTFCleaver_CreateJarProjectile)(vec, ang, vec2, vec3, thrower, info);
		if (proj != nullptr) {
			SetCustomProjectileModel(reinterpret_cast<CTFWeaponBaseGun *>(this),proj);
		}
		return proj;
	}

	DETOUR_DECL_MEMBER(CBaseAnimating *, CTFWeaponBaseGun_FireProjectile, CTFPlayer *player)
	{
		auto proj = DETOUR_MEMBER_CALL(CTFWeaponBaseGun_FireProjectile)(player);
		if (proj != nullptr) {
			auto weapon = reinterpret_cast<CTFWeaponBaseGun *>(this);
			SetCustomProjectileModel(weapon,proj);
		}
		return proj;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Bot:MultiClass_Weapon")
		{
			
			//MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_SecondaryAttack,     "CTFWeaponBase::SecondaryAttack");
			//MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_CanPerformSecondaryAttack,     "CTFWeaponBase::CanPerformSecondaryAttack");
			//MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_FireRocket,     "CTFWeaponBaseGun::FireRocket");
			//MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_FireFlare,     "CTFWeaponBaseGun::FireFlare");
			MOD_ADD_DETOUR_MEMBER(CTFJar_CreateJarProjectile,     "CTFJar::CreateJarProjectile");
			MOD_ADD_DETOUR_MEMBER(CTFJarMilk_CreateJarProjectile,     "CTFJarMilk::CreateJarProjectile");
			MOD_ADD_DETOUR_MEMBER(CTFJarGas_CreateJarProjectile,     "CTFJarGas::CreateJarProjectile");
			MOD_ADD_DETOUR_MEMBER(CTFCleaver_CreateJarProjectile,     "CTFCleaver::CreateJarProjectile");
			//MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_FireNail,     "CTFWeaponBaseGun::FireNail");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_FireProjectile,     "CTFWeaponBaseGun::FireProjectile");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_bot_multiclass_weapon", "0", FCVAR_NOTIFY,
		"Mod: remap item entity names so bots can be given multi-class weapons",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
