#include "mod.h"
#include "stub/baseentity.h"
#include "stub/projectiles.h"
#include "stub/entities.h"
#include "stub/misc.h"
#include "util/misc.h"


namespace Mod::Etc::Weapon_Mimic_Teamnum
{
    CBaseEntity *projectile = nullptr;
    RefCount rc_CTFPointWeaponMimic_Fire;
	CBaseEntity *scorer = nullptr;
	bool grenade = false;
	bool do_crits = false;
    DETOUR_DECL_MEMBER(void, CTFPointWeaponMimic_Fire)
	{
        SCOPED_INCREMENT(rc_CTFPointWeaponMimic_Fire);
		auto *mimic = reinterpret_cast<CTFPointWeaponMimic *>(this);
        projectile = nullptr;
		grenade = false;
		
		variant_t variant;
		int spawnflags = mimic->m_spawnflags;
		if (mimic->GetOwnerEntity() != nullptr && mimic->GetOwnerEntity()->IsPlayer()) {
			scorer = mimic->GetOwnerEntity();
		}
		if (mimic->m_nWeaponType == 4) {
			FireBulletsInfo_t info;

			QAngle vFireAngles = mimic->GetFiringAngles();
			Vector vForward;
			AngleVectors( vFireAngles, &vForward, nullptr, nullptr);

			info.m_vecSrc = mimic->GetAbsOrigin();
			info.m_vecDirShooting = vForward;
			info.m_iTracerFreq = 1;
			info.m_iShots = 1;
			info.m_pAttacker = mimic->GetOwnerEntity();
			if (info.m_pAttacker == nullptr) {
				info.m_pAttacker = mimic;
			}

			info.m_vecSpread = vec3_origin;

			if (mimic->m_flSpeedMax != 0.0f) {
				info.m_flDistance = mimic->m_flSpeedMax;
			}
			else {
				info.m_flDistance = 8192.0f;
			}
			info.m_iAmmoType = 1;//m_iAmmoType;
			info.m_flDamage = mimic->m_flDamage;
			info.m_flDamageForceScale = mimic->m_flSplashRadius;

			do_crits = mimic->m_bCrits;
			mimic->FireBullets(info);
			do_crits = false;
			projectile = nullptr;
			
		}
		else {
        	DETOUR_MEMBER_CALL(CTFPointWeaponMimic_Fire)();
		}

        if (projectile != nullptr) {
			// Set models to rockets and arrows
			//if (mimic->m_pzsModelOverride != NULL_STRING && (spawnflags & 2) && (mimic->m_nWeaponType == 0 || mimic->m_nWeaponType == 2)) {
			//	projectile->SetModel(mimic->m_pzsModelOverride);
			//}

            if (mimic->GetTeamNumber() != 0) {
				CBaseAnimating *anim = rtti_cast<CBaseAnimating *>(projectile);
                projectile->SetTeamNumber(mimic->GetTeamNumber());
				if (anim->m_nSkin == 1 && mimic->GetTeamNumber() == 2)
					anim->m_nSkin = 0;
			}
			if (grenade) {
				projectile->SetOwnerEntity(scorer);
			}
			// Fire callback
			if (spawnflags & 1) {
				variant_t variant;
            	variant.SetString(NULL_STRING);
                mimic->m_OnUser4->FireOutput(variant, projectile, mimic);
			}
			
			// Play sound, Particles
			if (spawnflags & 2) {
				string_t particle = mimic->m_pzsFireParticles;
				string_t sound = mimic->m_pzsFireSound;
				if (sound != NULL_STRING)
					mimic->EmitSound(STRING(sound));
				if (particle != NULL_STRING)
					DispatchParticleEffect(STRING(particle), PATTACH_ABSORIGIN_FOLLOW, mimic, INVALID_PARTICLE_ATTACHMENT, false);
			}
        }
		scorer = nullptr;
	}


    DETOUR_DECL_MEMBER(void, CBaseEntity_ModifyFireBulletsDamage, CTakeDamageInfo* dmgInfo)
	{
		if (do_crits) {
			dmgInfo->SetDamageType(dmgInfo->GetDamageType() | DMG_CRITICAL);
		}
        DETOUR_MEMBER_CALL(CBaseEntity_ModifyFireBulletsDamage)(dmgInfo);
	}

    DETOUR_DECL_STATIC(CBaseEntity *, CTFProjectile_Rocket_Create, CBaseEntity *pLauncher, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer)
	{
		if (scorer != nullptr) {
			pScorer = scorer;
		}
        projectile = DETOUR_STATIC_CALL(CTFProjectile_Rocket_Create)(pLauncher, vecOrigin, vecAngles, pOwner, pScorer);
        return projectile;
	}

    DETOUR_DECL_STATIC(CBaseEntity *, CTFProjectile_Arrow_Create, const Vector &vecOrigin, const QAngle &vecAngles, const float fSpeed, const float fGravity, int projectileType, CBaseEntity *pOwner, CBaseEntity *pScorer)
	{
		if (scorer != nullptr) {
			pScorer = scorer;
		}
        projectile = DETOUR_STATIC_CALL(CTFProjectile_Arrow_Create)(vecOrigin, vecAngles, fSpeed, fGravity, projectileType, pOwner, pScorer);
        return projectile;
	}
	
    DETOUR_DECL_STATIC(CBaseEntity *, CBaseEntity_CreateNoSpawn, const char *szName, const Vector& vecOrigin, const QAngle& vecAngles, CBaseEntity *pOwner)
	{
		if (scorer != nullptr) {
			grenade = true;
			pOwner = scorer;
		}
        projectile = DETOUR_STATIC_CALL(CBaseEntity_CreateNoSpawn)(szName, vecOrigin, vecAngles, pOwner);
        return projectile;
	}

	void OnRemove(CTFPointWeaponMimic *mimic)
	{
		for (int i = 0; i < mimic->m_Pipebombs->Count(); ++i) {
			auto proj = mimic->m_Pipebombs[i];
			if (proj != nullptr)
			//DevMsg("owner %d %d\n", proj->GetOwnerEntity(), mimic);
			//if (proj->GetOwnerEntity() == mimic)
				proj->Remove();
		}
	}

	DETOUR_DECL_MEMBER(void, CTFPointWeaponMimic_dtor0)
	{
		OnRemove(reinterpret_cast<CTFPointWeaponMimic *>(this));
		
        DETOUR_MEMBER_CALL(CTFPointWeaponMimic_dtor0)();
	}
	
	DETOUR_DECL_MEMBER(void, CTFPointWeaponMimic_dtor2)
	{
		OnRemove(reinterpret_cast<CTFPointWeaponMimic *>(this));
		
        DETOUR_MEMBER_CALL(CTFPointWeaponMimic_dtor2)();
	}

	DETOUR_DECL_MEMBER(void, CTFPointWeaponMimic_Spawn)
	{
        DETOUR_MEMBER_CALL(CTFPointWeaponMimic_Spawn)();
		auto mimic = reinterpret_cast<CTFPointWeaponMimic *>(this);
		
		int spawnflags = mimic->m_spawnflags;
		string_t sound = mimic->m_pzsFireSound;
		string_t particle = mimic->m_pzsFireParticles;
		if (spawnflags & 2) {
			if (sound != NULL_STRING) {
				if (!enginesound->PrecacheSound(STRING(sound), true))
					CBaseEntity::PrecacheScriptSound(STRING(sound));
			}

			if (particle != NULL_STRING)
				PrecacheParticleSystem(STRING(particle));
		}
		if ((spawnflags & 2) || mimic->m_nWeaponType == 4) {
			mimic->AddEFlags(EFL_FORCE_CHECK_TRANSMIT);
		}
	}

    class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Weapon_Mimic_Teamnum")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPointWeaponMimic_Fire, "CTFPointWeaponMimic::Fire");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_ModifyFireBulletsDamage, "CBaseEntity::ModifyFireBulletsDamage");
			MOD_ADD_DETOUR_STATIC(CTFProjectile_Rocket_Create,  "CTFProjectile_Rocket::Create");
			MOD_ADD_DETOUR_STATIC(CTFProjectile_Arrow_Create,  "CTFProjectile_Arrow::Create");
			MOD_ADD_DETOUR_STATIC(CBaseEntity_CreateNoSpawn,  "CBaseEntity::CreateNoSpawn");
			MOD_ADD_DETOUR_MEMBER(CTFPointWeaponMimic_dtor0,  "CTFPointWeaponMimic::~CTFPointWeaponMimic [D0]");
			MOD_ADD_DETOUR_MEMBER(CTFPointWeaponMimic_dtor2,  "CTFPointWeaponMimic::~CTFPointWeaponMimic [D2]");
			MOD_ADD_DETOUR_MEMBER(CTFPointWeaponMimic_Spawn,  "CTFPointWeaponMimic::Spawn");
		}
	};
	CMod s_Mod;

    ConVar cvar_enable("sig_etc_weapon_mimic_teamnum", "0", FCVAR_NOTIFY,
		"Mod: weapon mimic teamnum fix",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}