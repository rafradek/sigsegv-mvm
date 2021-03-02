#include "mod.h"
#include "stub/baseentity.h"
#include "stub/projectiles.h"


namespace Mod::Etc::Weapon_Mimic_Teamnum
{
    CBaseEntity *projectile = nullptr;
    RefCount rc_CTFPointWeaponMimic_Fire;
    DETOUR_DECL_MEMBER(void, CTFPointWeaponMimic_Fire)
	{
        SCOPED_INCREMENT(rc_CTFPointWeaponMimic_Fire);
		CBaseEntity *mimic = reinterpret_cast<CBaseEntity *>(this);
        projectile = nullptr;
        DETOUR_MEMBER_CALL(CTFPointWeaponMimic_Fire)();
        if (projectile != nullptr) {
            if (mimic->GetTeamNumber() != 0) {
				CBaseAnimating *anim = rtti_cast<CBaseAnimating *>(projectile);
                projectile->SetTeamNumber(mimic->GetTeamNumber());
				if (anim->m_nSkin == 1 && mimic->GetTeamNumber() == 2)
					anim->m_nSkin = 0;
			}
        }
	}

    DETOUR_DECL_STATIC(CBaseEntity *, CTFProjectile_Rocket_Create, CBaseEntity *pLauncher, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer)
	{
        projectile = DETOUR_STATIC_CALL(CTFProjectile_Rocket_Create)(pLauncher, vecOrigin, vecAngles, pOwner, pScorer);
        return projectile;
	}

    DETOUR_DECL_STATIC(CBaseEntity *, CTFProjectile_Arrow_Create, const Vector &vecOrigin, const QAngle &vecAngles, const float fSpeed, const float fGravity, int projectileType, CBaseEntity *pOwner, CBaseEntity *pScorer)
	{
        projectile = DETOUR_STATIC_CALL(CTFProjectile_Arrow_Create)(vecOrigin, vecAngles, fSpeed, fGravity, projectileType, pOwner, pScorer);
        return projectile;
	}

    DETOUR_DECL_STATIC(CBaseEntity *, CBaseEntity_CreateNoSpawn, const char *szName, const Vector& vecOrigin, const QAngle& vecAngles, CBaseEntity *pOwner)
	{
        projectile = DETOUR_STATIC_CALL(CBaseEntity_CreateNoSpawn)(szName, vecOrigin, vecAngles, pOwner);
        return projectile;
	}

    class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Weapon_Mimic_Teamnum")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPointWeaponMimic_Fire, "CTFPointWeaponMimic::Fire");
			MOD_ADD_DETOUR_STATIC(CTFProjectile_Rocket_Create,  "CTFProjectile_Rocket::Create");
			MOD_ADD_DETOUR_STATIC(CTFProjectile_Arrow_Create,  "CTFProjectile_Arrow::Create");
			MOD_ADD_DETOUR_STATIC(CBaseEntity_CreateNoSpawn,  "CBaseEntity::CreateNoSpawn");
		}
	};
	CMod s_Mod;

    ConVar cvar_enable("sig_etc_weapon_mimic_teamnum", "0", FCVAR_NOTIFY,
		"Mod: weapon mimic teamnum fix",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}