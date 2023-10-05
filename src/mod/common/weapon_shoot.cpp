#include "mod.h"
#include "stub/extraentitydata.h"
#include "stub/tfweaponbase.h"
#include "util/iterate.h"
#include "util/misc.h"
#include "stub/objects.h"
#include "stub/particles.h"
#include "stub/usermessages_sv.h"

namespace Mod::Common::Weapon_Shoot
{
    CTFWeaponBaseGun *shooting_weapon = nullptr;
    CBaseEntity *shooting_shooter = nullptr;
	CBaseEntity *player_replace = nullptr;
    RefCount rc_FireWeapon;
	RefCount rc_FireWeapon_RemoveAmmo;

	class ShooterWeaponModule : public EntityModule
	{
	public:
        ShooterWeaponModule() {}
        ShooterWeaponModule(CBaseEntity *entity) : EntityModule(entity) {}
        virtual ~ShooterWeaponModule() {
			for (auto proj : m_ProjectilesToRemove) {
				proj->Remove();
			}
		}
        void AddProjectileToRemove(CBaseEntity *entity) {
			m_ProjectilesToRemove.push_back(entity);
		}
	private:
		std::vector<CHandle<CBaseEntity>> m_ProjectilesToRemove;
	};

	GlobalThunk<int> m_nPredictionRandomSeedServer("CBaseEntity::m_nPredictionRandomSeedServer");
	GlobalThunk<int> m_nPredictionRandomSeed("CBaseEntity::m_nPredictionRandomSeed");
    CBaseEntity *FireWeapon(CBaseEntity *shooter, CTFWeaponBaseGun *weapon, const Vector &vecOrigin, const QAngle &vecAngles, bool isCrit, bool removeAmmo, int teamDefault)
    {
        if (weapon == nullptr) return nullptr;
        SCOPED_INCREMENT(rc_FireWeapon);
		SCOPED_INCREMENT_IF(rc_FireWeapon_RemoveAmmo, !removeAmmo);
        CBaseEntity *projectile = nullptr;
        bool tempPlayer = false;
        
        auto player = ToTFPlayer(shooter->IsBaseObject() ? ((CBaseObject *)shooter)->GetBuilder() : shooter->GetOwnerEntity());
        auto shooterTeam = shooter->GetTeamNumber() != 0 ? shooter->GetTeamNumber() : teamDefault;
        if (player == nullptr) {
            tempPlayer = true;
            
            CTFPlayer *anyTeamPlayer = nullptr;
            CTFPlayer *anySpecTeamPlayer = nullptr;
            ForEachTFPlayer([&](CTFPlayer *playerl){
                if (playerl->GetTeamNumber() == shooterTeam) {
                    player = playerl;
                }
                if (playerl->GetTeamNumber() < TF_TEAM_RED) {
                	anySpecTeamPlayer = playerl;
				}
                anyTeamPlayer = playerl;
            });
            if (player == nullptr) {
                player = anySpecTeamPlayer != nullptr ? anySpecTeamPlayer : anyTeamPlayer;
            }
        }
        if (player == nullptr) return nullptr;
        // Move the owner player to shooter position
        Vector oldPos = player->GetAbsOrigin();
        QAngle oldAng = player->EyeAngles();
        Vector oldPunchAngle = player->m_Local->m_vecPunchAngle;
		player->m_Local->m_vecPunchAngle = vec3_origin;
        player->pl->v_angle = vecAngles;
        Vector eyeOffset = player->EyePosition() - player->GetAbsOrigin();
        player->SetAbsOrigin(vecOrigin - eyeOffset);
        // Fire the weapon
        weapon->ChangeTeam(shooterTeam);
        if (weapon->GetOwner() != player) {
            weapon->SetOwnerEntity(player);
            weapon->SetOwner(player);
        }
        
        weapon->SetAbsOrigin(vecOrigin + Vector(0,0,1));
        weapon->SetAbsAngles(vecAngles);

		auto oldSolid = shooter->GetCollisionGroup();

        shooting_weapon = weapon;
        shooting_shooter = shooter;
		player_replace = player;
        auto oldActive = player->GetActiveWeapon();
        player->SetActiveWeapon(weapon);
        int oldTeam = player->GetTeamNumber();
        player->SetTeamNumber(shooter->GetTeamNumber());
        weapon->m_bCurrentAttackIsCrit = isCrit;
		shooter->SetCollisionGroup(COLLISION_GROUP_VEHICLE_CLIP);
		weapon->SetCollisionGroup(COLLISION_GROUP_VEHICLE_CLIP);
		m_nPredictionRandomSeedServer.GetRef() = gpGlobals->tickcount * 434;
		m_nPredictionRandomSeed.GetRef() = gpGlobals->tickcount * 434;
        projectile = weapon->FireProjectile(player);
		shooter->SetCollisionGroup(oldSolid);
        player->SetActiveWeapon(oldActive);
        player->SetTeamNumber(oldTeam);
        shooting_weapon = nullptr;
        shooting_shooter = nullptr;
		player_replace = nullptr;

        if (projectile != nullptr) {
            projectile->SetCustomVariable("shooterent", Variant(shooter));

            auto scorerInterface = rtti_cast<IScorer *>(projectile);
			// If the projectile has an IScorer interface, it is fine to make the shooter the owner of the projectile
			if (scorerInterface != nullptr && projectile->GetOwnerEntity() == player) {
				projectile->SetOwnerEntity(shooter);
			}
			// Dragon's fury projectiles must be removed when the weapon is removed
			if (rtti_cast<CTFProjectile_BallOfFire *>(projectile) != nullptr) {
				weapon->GetOrCreateEntityModule<ShooterWeaponModule>("shooterweaponmodule")->AddProjectileToRemove(projectile);
			}
        }
        if (tempPlayer) {
            weapon->SetOwnerEntity(nullptr);
            weapon->SetOwner(nullptr);
            if (projectile != nullptr) {
                projectile->SetOwnerEntity(shooter);
                projectile->ChangeTeam(shooterTeam);
                if (rtti_cast<CBaseGrenade *>(projectile) != nullptr) {
                    rtti_cast<CBaseGrenade *>(projectile)->SetThrower(shooter);
                }
                if (rtti_cast<CTFProjectile_Rocket *>(projectile) != nullptr)
                    rtti_cast<CTFProjectile_Rocket *>(projectile)->SetScorer(shooter);
                else if (rtti_cast<CTFBaseProjectile *>(projectile) != nullptr)
                    rtti_cast<CTFBaseProjectile *>(projectile)->SetScorer(shooter);
                else if (rtti_cast<CTFProjectile_Arrow *>(projectile) != nullptr)
                    rtti_cast<CTFProjectile_Arrow *>(projectile)->SetScorer(shooter);
                else if (rtti_cast<CTFProjectile_Flare *>(projectile) != nullptr)
                    rtti_cast<CTFProjectile_Flare *>(projectile)->SetScorer(shooter);
                else if (rtti_cast<CTFProjectile_EnergyBall *>(projectile) != nullptr)
                    rtti_cast<CTFProjectile_EnergyBall *>(projectile)->SetScorer(shooter);
            }
        }
        player->SetAbsOrigin(oldPos);
        player->pl->v_angle = oldAng;
        player->m_Local->m_vecPunchAngle = oldPunchAngle;
        return projectile;
    }

    DETOUR_DECL_MEMBER(bool, CTFGameRules_FPlayerCanTakeDamage, CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo& info)
	{
		if (pPlayer != nullptr && pAttacker != nullptr && pPlayer->GetTeamNumber() == pAttacker->GetTeamNumber()) {
			variant_t var;
			if (pAttacker == shooting_shooter && pPlayer != shooting_shooter->GetOwnerEntity()) {
				return false;
			}
			else if (info.GetInflictor() != nullptr && info.GetInflictor()->GetCustomVariableVariant<"shooterent">(var) && pAttacker == var.Entity() && pPlayer != var.Entity()->GetOwnerEntity()) {
				return false;
			}
		}
		
		return DETOUR_MEMBER_CALL(CTFGameRules_FPlayerCanTakeDamage)(pPlayer, pAttacker, info);
	}
	
	DETOUR_DECL_STATIC(CTFWeaponBase *, GetKilleaterWeaponFromDamageInfo, CTakeDamageInfo &info)
	{
		if (info.GetWeapon() != nullptr && info.GetWeapon()->MyCombatWeaponPointer() != nullptr && info.GetWeapon()->MyCombatWeaponPointer()->GetOwner() == nullptr) return nullptr;
		return DETOUR_STATIC_CALL(GetKilleaterWeaponFromDamageInfo)( info);
	}

	DETOUR_DECL_STATIC(void, EconItemInterface_OnOwnerKillEaterEvent_Batched, void *pEconInterface, class CTFPlayer *pOwner, class CTFPlayer *pVictim, int eEventType, int nIncrementValue)
	{
		if (pOwner == nullptr) return;
		DETOUR_STATIC_CALL(EconItemInterface_OnOwnerKillEaterEvent_Batched)(pEconInterface, pOwner, pVictim, eEventType, nIncrementValue);
	}

	DETOUR_DECL_STATIC(void, EconItemInterface_OnOwnerKillEaterEvent, void *pEconEntity, CTFPlayer *pOwner, CTFPlayer *pVictim, int eEventType, int nIncrementValue)
	{
		if (pOwner == nullptr) return;
		DETOUR_STATIC_CALL(EconItemInterface_OnOwnerKillEaterEvent)(pEconEntity, pOwner, pVictim, eEventType, nIncrementValue);
	}
	
	DETOUR_DECL_MEMBER(void, CTFPlayer_Event_Killed, const CTakeDamageInfo& info)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		DETOUR_MEMBER_CALL(CTFPlayer_Event_Killed)(info);
		CBaseEntity *observer = player->m_hObserverTarget;
		if (info.GetInflictor() != nullptr) {
            variant_t var;
            info.GetInflictor()->GetCustomVariableVariant<"shooterent">(var);
            if (var.Entity() != nullptr) {
                player->m_hObserverTarget = var.Entity();
            }
		}
		if (shooting_shooter != nullptr) {
			player->m_hObserverTarget = shooting_shooter;
		}
	}

	class CDmgAccumulator;
	DETOUR_DECL_MEMBER(void, CBaseEntity_DispatchTraceAttack, CTakeDamageInfo& info, const Vector& vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator)
	{
		if (shooting_weapon != nullptr && ToTFPlayer(shooting_shooter->GetOwnerEntity()) == nullptr) {
			info.SetAttacker(shooting_shooter);
			info.SetInflictor(shooting_weapon);
		}
		DETOUR_MEMBER_CALL(CBaseEntity_DispatchTraceAttack)(info, vecDir, ptr, pAccumulator);
	}

	DETOUR_DECL_MEMBER(void, CTFWeaponBaseGun_RemoveProjectileAmmo, CTFPlayer *player)
	{
		if (rc_FireWeapon_RemoveAmmo) return;
		DETOUR_MEMBER_CALL(CTFWeaponBaseGun_RemoveProjectileAmmo)(player);
	}

	DETOUR_DECL_MEMBER(void, CTFWeaponBaseGun_UpdatePunchAngles, CTFPlayer *player)
	{
		if (rc_FireWeapon) return;

		DETOUR_MEMBER_CALL(CTFWeaponBaseGun_UpdatePunchAngles)(player);
	}

	DETOUR_DECL_MEMBER(void, CTFWeaponBaseGun_DoFireEffects)
	{
		if (rc_FireWeapon) return;

		DETOUR_MEMBER_CALL(CTFWeaponBaseGun_DoFireEffects)();
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_DoAnimationEvent, PlayerAnimEvent_t event, int data)
	{
		if (rc_FireWeapon) return;

		DETOUR_MEMBER_CALL(CTFPlayer_DoAnimationEvent)(event, data);
	}

	DETOUR_DECL_MEMBER(void, CTFProjectile_Flare_SendDeathNotice)
	{
		auto flare = reinterpret_cast<CTFProjectile_Flare *>(this);
		if (flare->GetOwnerEntity() != nullptr && ToTFPlayer(flare->GetOwnerEntity()) == nullptr) return;

		DETOUR_MEMBER_CALL(CTFProjectile_Flare_SendDeathNotice)();
	}

	DETOUR_DECL_MEMBER(void, CBaseCombatWeapon_WeaponSound, int index, float soundtime) 
	{
		if (shooting_weapon != nullptr && shooting_shooter != nullptr) {
			if (shooting_shooter->GetCustomVariableFloat<"weaponnosound">()) {
				return;
			}
			auto me = reinterpret_cast<CBaseCombatWeapon *>(this);
			auto oldOwner = me->GetOwner();
			me->SetOwner(nullptr);
			DETOUR_MEMBER_CALL(CBaseCombatWeapon_WeaponSound)(index, soundtime);
			me->SetOwner(oldOwner);
		}
		else {
			DETOUR_MEMBER_CALL(CBaseCombatWeapon_WeaponSound)(index, soundtime);
		}
	}

	
	bool dragons_fury_burning = false;
	DETOUR_DECL_MEMBER(int, CTFGameRules_ApplyOnDamageModifyRules, CTakeDamageInfo& info, CBaseEntity *pVictim, bool b1)
	{
		// For shooter weapons, calculate damage falloff from the shooter
        auto shooter = shooting_shooter;
		if (shooter != nullptr) {
			info.SetInflictor(shooter);
            auto obj = ToBaseObject(shooter);
			if ((obj == nullptr && shooter->GetOwnerEntity() == nullptr) || (obj != nullptr && obj->GetBuilder() == nullptr)) {
				info.SetAttacker(shooter);
			}
		}

		// For non player owner dragon's fury, implement 3x damage bonus here
		if (dragons_fury_burning) {
			info.SetDamage(info.GetDamage() * 3);
		}

        auto result = DETOUR_MEMBER_CALL(CTFGameRules_ApplyOnDamageModifyRules)(info, pVictim, b1);

		if (info.GetAttacker() == nullptr || !info.GetAttacker()->IsPlayer()) {
			auto playerVictim = ToTFPlayer(pVictim);
			auto projInflictor = rtti_cast<CBaseProjectile *>(info.GetInflictor());
			if (playerVictim != nullptr && projInflictor != nullptr) {
				auto weaponProj = rtti_cast<CTFWeaponBase *>(projInflictor->GetOriginalLauncher());
				if (weaponProj != nullptr && weaponProj->GetAfterburnRateOnHit() > 0) {
					playerVictim->m_Shared->Burn(playerVictim, weaponProj, weaponProj->GetAfterburnRateOnHit());
				}
			}
		}
		return result;
    }

	// DETOUR_DECL_STATIC_CALL_CONVENTION(__gcc_regcall, CGameTrace *, UTIL_PlayerBulletTrace, Vector &vec1, Vector &vec2, Vector &vec3, uint val1, ITraceFilter *filter, CGameTrace *trace)
	// {
	// 	Msg("Trace\n");
	// 	if (rc_FireWeapon) {
	// 		auto simpleFilter = rtti_cast<CTraceFilterSimple *>(filter);
	// 		if (simpleFilter != nullptr) {
	// 			simpleFilter->SetPassEntity(shooting_shooter);
	// 		}
	// 		Msg("Trace do %d %d\n", filter, simpleFilter);
	// 	}

    //     return DETOUR_STATIC_CALL(UTIL_PlayerBulletTrace)(vec1, vec2, vec3, val1, filter, trace);
    // }

	DETOUR_DECL_MEMBER(void, CTraceFilterSimple_CTraceFilterSimple, const IHandleEntity *passentity, int collisionGroup, ShouldHitFunc_t pExtraShouldHitCheckFn)
	{
		if (passentity == player_replace) {
			passentity = shooting_shooter;
		}

        return DETOUR_MEMBER_CALL(CTraceFilterSimple_CTraceFilterSimple)(passentity, collisionGroup, pExtraShouldHitCheckFn);
    }

	DETOUR_DECL_MEMBER(void, CTFProjectile_BallOfFire_Burn, CBaseEntity *entity)
	{
		auto player = ToTFPlayer(entity);
		auto owner = reinterpret_cast<CTFProjectile_BallOfFire *>(this)->GetOwnerEntity();
		if ((owner == nullptr || !owner->IsPlayer())) {
			reinterpret_cast<CTFProjectile_BallOfFire *>(this)->m_bLandedBonusDamage = true;
		}
        DETOUR_MEMBER_CALL(CTFProjectile_BallOfFire_Burn)(entity);
    }

	DETOUR_DECL_STATIC(void, TE_FireBullets, int iPlayerIndex, const Vector &vOrigin, const QAngle &vAngles, 
					 int iWeaponID, int	iMode, int iSeed, float flSpread, bool bCritical)
	{
		if (rc_FireWeapon) return;
        DETOUR_STATIC_CALL(TE_FireBullets)(iPlayerIndex, vOrigin, vAngles, iWeaponID, iMode, iSeed, flSpread, bCritical);
	}

	int customDamageTypeBullet = 0;
	RefCount rc_FireWeaponDoTrace;
	DETOUR_DECL_MEMBER(void, CTFPlayer_FireBullet, CTFWeaponBase *weapon, FireBulletsInfo_t& info, bool bDoEffects, int nDamageType, int nCustomDamageType)
	{
		bool doTrace = false;
		if (rc_FireWeapon) {
			static int	tracerCount;
			bDoEffects = true;
			int ePenetrateType = weapon ? weapon->GetPenetrateType() : TF_DMG_CUSTOM_NONE;
			if (ePenetrateType == TF_DMG_CUSTOM_NONE)
				ePenetrateType = customDamageTypeBullet;
			doTrace = ( ( info.m_iTracerFreq != 0 ) && ( tracerCount++ % info.m_iTracerFreq ) == 0 ) || (ePenetrateType == TF_DMG_CUSTOM_PENETRATE_ALL_PLAYERS);
		}

		SCOPED_INCREMENT_IF(rc_FireWeaponDoTrace, doTrace);

		
		DETOUR_MEMBER_CALL(CTFPlayer_FireBullet)(weapon, info, bDoEffects, nDamageType, nCustomDamageType);
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_MaybeDrawRailgunBeam, IRecipientFilter *filter, CTFWeaponBase *weapon, const Vector &vStartPos, const Vector &vEndPos)
	{
		if (rc_FireWeaponDoTrace) {
			te_tf_particle_effects_control_point_t cp;
			cp.m_eParticleAttachment = PATTACH_ABSORIGIN;
			cp.m_vecOffset = vEndPos;
			DispatchParticleEffect(reinterpret_cast<CTFPlayer *>(this)->GetTracerType(), PATTACH_ABSORIGIN, nullptr, nullptr, vStartPos, true, vec3_origin, vec3_origin, false, false, &cp, nullptr);
		}
		DETOUR_MEMBER_CALL(CTFPlayer_MaybeDrawRailgunBeam)(filter, weapon, vStartPos, vEndPos);
	}

	DETOUR_DECL_MEMBER(void, CLagCompensationManager_StartLagCompensation, CBasePlayer *player, CUserCmd *cmd)
	{
		if (cmd == nullptr) return;

		DETOUR_MEMBER_CALL(CLagCompensationManager_StartLagCompensation)(player, cmd);
	}


    class CMod : public IMod
    {
    public:
        CMod() : IMod("Common:Weapon_Shoot")
        {
            //MOD_ADD_DETOUR_MEMBER(CBaseEntity_CBaseEntity, "CBaseEntity::CBaseEntity");

			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_RemoveProjectileAmmo,  "CTFWeaponBaseGun::RemoveProjectileAmmo");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_UpdatePunchAngles,  "CTFWeaponBaseGun::UpdatePunchAngles");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_DoFireEffects,  "CTFWeaponBaseGun::DoFireEffects");

			MOD_ADD_DETOUR_STATIC(GetKilleaterWeaponFromDamageInfo,  "GetKilleaterWeaponFromDamageInfo");
			MOD_ADD_DETOUR_STATIC(EconItemInterface_OnOwnerKillEaterEvent_Batched,  "EconItemInterface_OnOwnerKillEaterEvent_Batched");
			MOD_ADD_DETOUR_STATIC(EconItemInterface_OnOwnerKillEaterEvent,  "EconItemInterface_OnOwnerKillEaterEvent");
            
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_Event_Killed,  "CTFPlayer::Event_Killed");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_DispatchTraceAttack,    "CBaseEntity::DispatchTraceAttack");
			MOD_ADD_DETOUR_MEMBER(CBaseCombatWeapon_WeaponSound,    "CBaseCombatWeapon::WeaponSound");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_DoAnimationEvent,    "CTFPlayer::DoAnimationEvent");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_ApplyOnDamageModifyRules, "CTFGameRules::ApplyOnDamageModifyRules");

			// Fix non player owners of projectiles causing crashes
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Flare_SendDeathNotice,    "CTFProjectile_Flare::SendDeathNotice");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_BallOfFire_Burn,    "CTFProjectile_BallOfFire::Burn");

			MOD_ADD_DETOUR_MEMBER(CTFGameRules_FPlayerCanTakeDamage,    "CTFGameRules::FPlayerCanTakeDamage");

			// Bullet tracers
			MOD_ADD_DETOUR_STATIC(TE_FireBullets,  "TE_FireBullets");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_FireBullet,    "CTFPlayer::FireBullet");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_MaybeDrawRailgunBeam,    "CTFPlayer::MaybeDrawRailgunBeam");

			// Fix lag compensation crash
			MOD_ADD_DETOUR_MEMBER(CLagCompensationManager_StartLagCompensation, "CLagCompensationManager::StartLagCompensation");
			//MOD_ADD_DETOUR_STATIC(UTIL_ParticleTracer,    "UTIL_ParticleTracer");

			//MOD_ADD_DETOUR_MEMBER(CTraceFilterSimple_CTraceFilterSimple,    "CTraceFilterSimple::CTraceFilterSimple");
			//MOD_ADD_DETOUR_MEMBER(IEngineTrace_TraceRay,    "IEngineTrace::TraceRay");
			//MOD_ADD_DETOUR_STATIC(UTIL_PlayerBulletTrace,    "UTIL_PlayerBulletTrace");
			
        }
    };
    CMod s_Mod;
}