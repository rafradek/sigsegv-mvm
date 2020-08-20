#include "mod.h"
#include "stub/baseentity.h"
#include "stub/econ.h"
#include "stub/tfbot.h"
#include "util/scope.h"
#include "stub/projectiles.h"


namespace Mod::Attr::CustomProjectileModel_Precache
{
	DETOUR_DECL_MEMBER(void, CTFWeaponBaseGun_GetCustomProjectileModel, CAttribute_String *attr_str)
	{
		DETOUR_MEMBER_CALL(CTFWeaponBaseGun_GetCustomProjectileModel)(attr_str);
		
		const char *model_path = nullptr;
		CopyStringAttributeValueToCharPointerOutput(attr_str, &model_path);
		if (model_path != nullptr && *model_path != '\x00') {
			CBaseEntity::PrecacheModel(model_path);
		}
	}
	
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
		CMod() : IMod("Attr:CustomProjectileModel_Precache")
		{
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_GetCustomProjectileModel, "CTFWeaponBaseGun::GetCustomProjectileModel");
			MOD_ADD_DETOUR_MEMBER(CTFJar_CreateJarProjectile,     "CTFJar::CreateJarProjectile");
			MOD_ADD_DETOUR_MEMBER(CTFJarMilk_CreateJarProjectile,     "CTFJarMilk::CreateJarProjectile");
			MOD_ADD_DETOUR_MEMBER(CTFJarGas_CreateJarProjectile,     "CTFJarGas::CreateJarProjectile");
			MOD_ADD_DETOUR_MEMBER(CTFCleaver_CreateJarProjectile,     "CTFCleaver::CreateJarProjectile");
			//MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_FireNail,     "CTFWeaponBaseGun::FireNail");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_FireProjectile,     "CTFWeaponBaseGun::FireProjectile");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_attr_customprojectilemodel_precache", "0", FCVAR_NOTIFY,
		"Mod: do automatic model precaching of \"custom projectile model\" attr values instead of crashing the server",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
