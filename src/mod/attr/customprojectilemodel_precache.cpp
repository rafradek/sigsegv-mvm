#include "mod.h"
#include "stub/baseentity.h"
#include "stub/econ.h"
#include "stub/tfbot.h"
#include "util/scope.h"
#include "stub/projectiles.h"


namespace Mod::Attr::CustomProjectileModel_Precache
{
	std::map<CHandle<CBaseAnimating>, float> projs;

	DETOUR_DECL_MEMBER(void, CTFWeaponBaseGun_GetCustomProjectileModel, CAttribute_String *attr_str)
	{
		DETOUR_MEMBER_CALL(CTFWeaponBaseGun_GetCustomProjectileModel)(attr_str);
		
		const char *model_path = nullptr;
		CopyStringAttributeValueToCharPointerOutput(attr_str, &model_path);
		if (model_path != nullptr && *model_path != '\x00') {
			CBaseEntity::PrecacheModel(model_path);
		}
	}
	
	void SetCustomProjectileModel(CBaseCombatWeapon *weapon, CBaseAnimating *proj){
		CAttributeList &attrlist = weapon->GetItem()->GetAttributeList();
		static int attr_def = GetItemSchema()->GetAttributeDefinitionByName("custom projectile model")->GetIndex();
		auto attr = attrlist.GetAttributeByID(attr_def);
		if (attr != nullptr) {
			const char *modelname = nullptr;
			//GetItemSchema()->GetAttributeDefinitionByName("custom projectile model")->ConvertValueToString(*(attr->GetValuePtr()),modelname,255);
			CopyStringAttributeValueToCharPointerOutput(attr->GetValuePtr()->m_String, &modelname);
			proj->SetModel(modelname);
			// DevMsg("Setting custom projectile model to %s \n",modelname);
		}
		float modelscale = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( weapon, modelscale, mult_projectile_scale);
		if (modelscale != 1.0f)
			proj->SetModelScale(modelscale);
		float collscale = 0.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( weapon, collscale, custom_projectile_size);
		if (collscale != 0.0f) {
			Vector min = Vector(-collscale, -collscale, -collscale);
			Vector max = Vector(collscale, collscale, collscale);
			UTIL_SetSize(proj, &min, &max);
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

	DETOUR_DECL_MEMBER(void, CBaseProjectile_SetLauncher, CBaseEntity *launcher)
	{
		auto proj = reinterpret_cast<CBaseProjectile *>(this);
		CBaseEntity *original = proj->GetOriginalLauncher();
		DETOUR_MEMBER_CALL(CBaseProjectile_SetLauncher)(launcher);
		if (original == nullptr && launcher != nullptr && launcher->MyCombatWeaponPointer() != nullptr) {
			SetCustomProjectileModel(launcher->MyCombatWeaponPointer(),proj);
		}
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
	
	
	class CMod : public IMod//, public IModCallbackListener
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
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFWeaponBaseGun_FireProjectile,     "CTFWeaponBaseGun::FireProjectile", LOWEST);
			MOD_ADD_DETOUR_MEMBER(CBaseProjectile_SetLauncher,     "CBaseProjectile::SetLauncher");
			
		}

		/*virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			for (auto it = projs.begin(); it != projs.end(); it++) {
				if (it->first != nullptr) {
					if (it->second > 0.f)
						it->second = -(it.second);
					else {
						it->first->UpdateCollisionBounds();
						it->first->SetModelScale(-(it->second));
						it = projs.erase(it);
					}

				}
				else
					it = projs.erase(it);
			}
		}*/
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_attr_customprojectilemodel_precache", "0", FCVAR_NOTIFY,
		"Mod: do automatic model precaching of \"custom projectile model\" attr values instead of crashing the server",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
