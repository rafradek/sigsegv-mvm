#include "mod.h"
#include "stub/baseentity.h"
#include "stub/tfweaponbase.h"
#include "stub/projectiles.h"
#include "stub/objects.h"
#include "stub/tfentities.h"
#include "stub/nextbot_cc.h"
#include "stub/gamerules.h"
#include "stub/nav.h"
#include "stub/misc.h"
#include "util/backtrace.h"
#include "util/clientmsg.h"
#include "util/misc.h"
#include "util/iterate.h"
#include "tier1/CommandBuffer.h"


namespace Mod::Etc::Misc
{

	DETOUR_DECL_MEMBER(bool, CTFProjectile_Rocket_IsDeflectable)
	{
		auto ent = reinterpret_cast<CTFProjectile_Rocket *>(this);

		if (TFGameRules()->IsMannVsMachineMode() && strcmp(ent->GetClassname(), "tf_projectile_balloffire") == 0) {
			return false;
		}

		return DETOUR_MEMBER_CALL(CTFProjectile_Rocket_IsDeflectable)();
	}

	bool AllowHit(CBaseEntity *proj, CBaseEntity *other)
	{
		bool penetrates = proj->CollisionProp()->IsSolidFlagSet(FSOLID_TRIGGER);
		if (penetrates && !(other->entindex() == 0 || other->MyCombatCharacterPointer() != nullptr || other->IsCombatItem())) {
			Vector start = proj->GetAbsOrigin();
			Vector vel = proj->GetAbsVelocity();
			Vector end = start + vel * gpGlobals->frametime;
			trace_t tr;
			UTIL_TraceLine( start, end, MASK_SOLID, proj, COLLISION_GROUP_NONE, &tr );
			if (tr.m_pEnt == nullptr) {
				return false;
			}
		}
		return true;
	}

	DETOUR_DECL_MEMBER(void, CTFProjectile_Arrow_ArrowTouch, CBaseEntity *pOther)
	{
		auto arrow = reinterpret_cast<CTFProjectile_Arrow *>(this);
		
		if (AllowHit(arrow, pOther))
			DETOUR_MEMBER_CALL(CTFProjectile_Arrow_ArrowTouch)(pOther);
	}

	DETOUR_DECL_MEMBER(void, CTFProjectile_EnergyRing_ProjectileTouch, CBaseEntity *pOther)
	{
		auto proj = reinterpret_cast<CTFProjectile_EnergyRing *>(this);
		
		if (AllowHit(proj, pOther))
			DETOUR_MEMBER_CALL(CTFProjectile_EnergyRing_ProjectileTouch)(pOther);
	}

	DETOUR_DECL_MEMBER(void, CTFProjectile_BallOfFire_RocketTouch, CBaseEntity *pOther)
	{
		auto arrow = reinterpret_cast<CBaseEntity *>(this);
		
		if (AllowHit(arrow, pOther))
			DETOUR_MEMBER_CALL(CTFProjectile_BallOfFire_RocketTouch)(pOther);
	}

	RefCount rc_DoNotOverrideSmoke;
	CBaseEntity *sentry_attacker_rocket = nullptr;
	DETOUR_DECL_MEMBER(void, CTFBaseRocket_Explode, trace_t *pTrace, CBaseEntity *pOther)
	{
		auto proj = reinterpret_cast<CTFBaseRocket *>(this);

		auto owner = proj->GetOwnerEntity();
		sentry_attacker_rocket = nullptr;
		if (owner != nullptr && owner->IsBaseObject()) {
			if (ToBaseObject(owner)->GetBuilder() == nullptr) {
				proj->SetOwnerEntity(GetContainingEntity(INDEXENT(0)));
				sentry_attacker_rocket = owner;
			}
		}
		auto playerOwner = ToTFPlayer(owner);
		SCOPED_INCREMENT_IF(rc_DoNotOverrideSmoke, playerOwner != nullptr && FindCaseInsensitive(playerOwner->GetPlayerName(), "smoke") != nullptr);
		DETOUR_MEMBER_CALL(CTFBaseRocket_Explode)(pTrace, pOther);
		sentry_attacker_rocket = nullptr;
	}

	DETOUR_DECL_MEMBER(void, CTFGameRules_RadiusDamage, CTFRadiusDamageInfo& info)
	{
		if (sentry_attacker_rocket != nullptr && info.m_DmgInfo != nullptr) {
			info.m_DmgInfo->SetAttacker(sentry_attacker_rocket);
			sentry_attacker_rocket = nullptr;
		}
		DETOUR_MEMBER_CALL(CTFGameRules_RadiusDamage)(info);
	}
	
	RefCount rc_SendProxy_PlayerObjectList;
	bool playerobjectlist = false;
	DETOUR_DECL_STATIC(void, SendProxy_PlayerObjectList, const void *pProp, const void *pStruct, const void *pData, void *pOut, int iElement, int objectID)
	{
		SCOPED_INCREMENT(rc_SendProxy_PlayerObjectList);
		bool firstminisentry = true;
		CTFPlayer *player = (CTFPlayer *)(pStruct);
		for (int i = 0; i <= iElement; i++) {
			if (i < player->GetObjectCount()) {
				CBaseObject *obj = player->GetObject(i);
				if (obj != nullptr && obj->m_bDisposableBuilding) {
					if (!firstminisentry)
						iElement++;
					firstminisentry = false;
				}
			}
		}
		DETOUR_STATIC_CALL(SendProxy_PlayerObjectList)(pProp, pStruct, pData, pOut, iElement, objectID);
	}

	RefCount rc_SendProxyArrayLength_PlayerObjects;
	DETOUR_DECL_STATIC(int, SendProxyArrayLength_PlayerObjects, const void *pStruct, int objectID)
	{
		int count = DETOUR_STATIC_CALL(SendProxyArrayLength_PlayerObjects)(pStruct, objectID);
		CTFPlayer *player = (CTFPlayer *)(pStruct);
		bool firstminisentry = true;
		for (int i = 0; i < count; i++) {
			if (i < player->GetObjectCount()) {
				CBaseObject *obj = player->GetObject(i);
				if (obj != nullptr && obj->m_bDisposableBuilding) {
					if (!firstminisentry)
						count--;
					firstminisentry = false;
				}
			}
		}
		return count;
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_StartBuildingObjectOfType, int type, int mode)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (type == 2) {
			int buildcount = player->GetObjectCount();
			int sentrycount = 0;
			int maxdisposables = 0;
			
			int minanimtimeid = -1;
			float minanimtime = std::numeric_limits<float>::max();
			CALL_ATTRIB_HOOK_INT_ON_OTHER(player, maxdisposables, engy_disposable_sentries);

			for (int i = buildcount - 1; i >= 0; i--) {
				CBaseObject *obj = player->GetObject(i);
				if (obj != nullptr && obj->GetType() == OBJ_SENTRYGUN) {
					sentrycount++;
					if (obj->m_flSimulationTime < minanimtime && obj->m_bDisposableBuilding) {
						minanimtime = obj->m_flSimulationTime;
						minanimtimeid = i;
					}
				}
			}
			if (sentrycount >= maxdisposables + 1 && minanimtimeid != -1) {
				player->GetObject(minanimtimeid)->DetonateObject();
			}
		}
		DETOUR_MEMBER_CALL(CTFPlayer_StartBuildingObjectOfType)(type, mode);
	}

	DETOUR_DECL_MEMBER(void, CCaptureFlag_Drop, CTFPlayer *pPlayer, bool bVisible, bool bThrown, bool bMessage)
	{
		DETOUR_MEMBER_CALL(CCaptureFlag_Drop)(pPlayer, bVisible, bThrown, bMessage);
		auto flag = reinterpret_cast<CCaptureFlag *>(this);
		if ( TFGameRules()->IsMannVsMachineMode() )
		{
			Vector pos = flag->GetAbsOrigin();
			if (pPlayer != nullptr && TheNavMesh->GetNavArea(pos, 99999.9f ) == nullptr) {
				CNavArea *area = pPlayer->GetLastKnownArea();
				if (area != nullptr) {
					area->GetClosestPointOnArea( pos, &pos );
					pos.z += 5.0f;

					flag->SetAbsOrigin( pos );
				}
			}
		}

	}

	DETOUR_DECL_MEMBER(void, CFuncIllusionary_Spawn)
	{
		auto entity = reinterpret_cast<CBaseEntity *>(this);
		CBaseEntity::PrecacheModel(STRING(entity->GetModelName()));
		DETOUR_MEMBER_CALL(CFuncIllusionary_Spawn)();
	}	
	
	DETOUR_DECL_MEMBER(void, CTFPlayerShared_OnRemoveStunned)
	{
		auto shared = reinterpret_cast<CTFPlayerShared *>(this);
		auto weapon = shared->GetOuter()->GetActiveTFWeapon();
		if (weapon != nullptr) {
			weapon->RemoveEffects(EF_NODRAW);
		}
		DETOUR_MEMBER_CALL(CTFPlayerShared_OnRemoveStunned)();
	}
	
	DETOUR_DECL_MEMBER(CBaseEntity *, CTFPipebombLauncher_FireProjectile, CTFPlayer *player)
	{
		auto proj = DETOUR_MEMBER_CALL(CTFPipebombLauncher_FireProjectile)(player);
		auto weapon = reinterpret_cast<CTFPipebombLauncher *>(this);
		if (rtti_cast<CTFGrenadePipebombProjectile *>(proj) == nullptr) {
			weapon->m_Pipebombs->RemoveAll();
			weapon->m_iPipebombCount = 0;
		}
		return proj;
	}
	
	DETOUR_DECL_MEMBER(void, CTFPlayer_RemoveAllWeapons)
	{
		DETOUR_MEMBER_CALL(CTFPlayer_RemoveAllWeapons)();
		auto player = reinterpret_cast<CTFPlayer *>(this);
		player->ClearDisguiseWeaponList();
		if (player->m_Shared->m_hDisguiseWeapon != nullptr) {
			player->m_Shared->m_hDisguiseWeapon->Remove();
			player->m_Shared->m_hDisguiseWeapon = nullptr;
		}
	}

	VHOOK_DECL(bool, CZombie_ShouldCollide, int collisionGroup, int contentsMask)
    {
        if (collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT) {
            return false;
        }
        return VHOOK_CALL(CZombie_ShouldCollide)(collisionGroup,contentsMask );
    }

	DETOUR_DECL_MEMBER(void, CTFPlayer_Event_Killed, const CTakeDamageInfo& info)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		DETOUR_MEMBER_CALL(CTFPlayer_Event_Killed)(info);
        if (player->m_hObserverTarget != nullptr && !player->IsBot()) {
			ForEachEntityByClassname("ambient_generic", [&](CBaseEntity *pEnt){
				auto ambient = static_cast<CAmbientGeneric *>(pEnt);
				if (ambient->m_fActive && StringEndsWith(STRING(ambient->m_iszSound.Get()), ".mp3")) {
            		player->m_hObserverTarget = nullptr;
					return false;
				}
				return true;
			});
        }
	}

    class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Misc")
		{
            // Make dragons fury projectile non reflectable in mvm
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Rocket_IsDeflectable, "CTFProjectile_Rocket::IsDeflectable");

			// Make unowned sentry rocket deal damage
			MOD_ADD_DETOUR_MEMBER(CTFBaseRocket_Explode,    "CTFBaseRocket::Explode");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_RadiusDamage, "CTFGameRules::RadiusDamage");

			// Makes penetration arrows not collide with bounding boxes of various entities
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Arrow_ArrowTouch, "CTFProjectile_Arrow::ArrowTouch");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_EnergyRing_ProjectileTouch, "CTFProjectile_EnergyRing::ProjectileTouch");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_BallOfFire_RocketTouch, "CTFProjectile_BallOfFire::RocketTouch");

			// Allow to construct disposable sentries by destroying the oldest ones
			MOD_ADD_DETOUR_STATIC(SendProxy_PlayerObjectList,    "SendProxy_PlayerObjectList");
			MOD_ADD_DETOUR_STATIC(SendProxyArrayLength_PlayerObjects,    "SendProxyArrayLength_PlayerObjects");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_StartBuildingObjectOfType, "CTFPlayer::StartBuildingObjectOfType");

			// Drop flag to last bot nav area
			MOD_ADD_DETOUR_MEMBER(CCaptureFlag_Drop, "CCaptureFlag::Drop");

			// Fix cfuncillusionary crash
			MOD_ADD_DETOUR_MEMBER(CFuncIllusionary_Spawn, "CFuncIllusionary::Spawn");

			// Fix stun disappearing weapons
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_OnRemoveStunned, "CTFPlayerShared::OnRemoveStunned");

			// Fix arrow crash when firing too fast
			MOD_ADD_DETOUR_MEMBER(CTFPipebombLauncher_FireProjectile, "CTFPipebombLauncher::FireProjectile");

			// Remove inactive disguise weapons on death
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_RemoveAllWeapons, "CTFPlayer::RemoveAllWeapons");

			// Remove collision with zombies
			MOD_ADD_VHOOK(CZombie_ShouldCollide, TypeName<CZombie>(), "CBaseEntity::ShouldCollide");

			// No deathcam if music is playing
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_Event_Killed, "CTFPlayer::Event_Killed");
			
		}
	};
	CMod s_Mod;

    ConVar cvar_enable("sig_etc_misc", "0", FCVAR_NOTIFY,
		"Mod:\nDisables reflection of Dragon's Fury projectiles in MvM gamemode"
		"\nAutomatically destroy oldest disposable sentry when building new one"
		"\nFixes:"
		"\nSnap unreachable dropped mvm bomb to last nav area"
		"\nFix unowned sentry's rockets not dealing damage"
		"\nFix penetration arrows colliding with bounding boxes of various entities"
		"\nFix specific CFuncIllusionary crash"
		"\nFix stun disappearing weapons"
		"\nFix crash with firing too many arrows"
		"\nFix crash when trying to detonate non grenades on pipebomb launcher",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});


	extern ConVar sig_etc_max_total_script_files_count;
	DETOUR_DECL_STATIC(void, Script_StringToFile, const char *filename, const char *string)
	{
		// Delete oldest files if over the limit
		char filepath[512];
		snprintf(filepath, sizeof(filepath), "%s/scriptdata/%s",g_SMAPI->GetBaseDir(), filename);
		struct stat stats;
		std::string oldestFile;
		time_t oldestFileTime = LONG_MAX;
		int fileCount = 0;

		// If a new file is created, do the over the limit check
		if (stat(filepath, &stats) != 0) {
			char path[512];
			snprintf(path, sizeof(path), "%s/scriptdata", g_SMAPI->GetBaseDir());
			DIR *dir;
			dirent *ent;

			if ((dir = opendir(path)) != nullptr) {

				// Count the files
				while ((ent = readdir(dir)) != nullptr) {
					fileCount++;
				}
				rewinddir(dir);
				
				// Delete 10% of the files if getting over the limit
				if (fileCount > sig_etc_max_total_script_files_count.GetInt()) {
					std::vector<std::pair<std::string, time_t>> scriptNameAndModify;
					
					while ((ent = readdir(dir)) != nullptr) {
						if (ent->d_type != DT_DIR) {
							snprintf(filepath, sizeof(filepath), "%s/%s", path, ent->d_name);
							stat(filepath, &stats);
							time_t time = stats.st_mtim.tv_sec;
							scriptNameAndModify.push_back({filepath, time});
						}
					}
					std::sort(scriptNameAndModify.begin(), scriptNameAndModify.end(), [](std::pair<std::string, time_t> &pair1, std::pair<std::string, time_t> &pair2){
						return pair1.second < pair2.second;
					});
					scriptNameAndModify.resize(sig_etc_max_total_script_files_count.GetInt() / 10 + 1);
					for (auto &pair : scriptNameAndModify) {
						unlink(pair.first.c_str());
					}
					
				}
				closedir(dir);
			}
		}
		DETOUR_STATIC_CALL(Script_StringToFile)(filename, string);
	}

	class CMod_ScriptSaveLimit : public IMod
	{
	public:
		CMod_ScriptSaveLimit() : IMod("Etc:Misc:ScriptSaveLimit")
		{
			// Limit string to file
			MOD_ADD_DETOUR_STATIC(Script_StringToFile, "Script_StringToFile");
		}
	};
	CMod_ScriptSaveLimit s_ModScriptSaveLimit;

	ConVar sig_etc_max_total_script_files_count("sig_etc_max_total_script_files_count", "0", FCVAR_NOTIFY,
		"Mod: Max total count of files created by vscript",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_ModScriptSaveLimit.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});

	DETOUR_DECL_MEMBER(void, CTFWeaponBaseGrenadeProj_Explode, trace_t *pTrace, int bitsDamageType)
	{
		auto proj = reinterpret_cast<CTFWeaponBaseGrenadeProj *>(this);
		auto thrower = ToTFPlayer(proj->GetThrower());
		SCOPED_INCREMENT_IF(rc_DoNotOverrideSmoke, thrower != nullptr && FindCaseInsensitive(thrower->GetPlayerName(), "smoke") != nullptr);
		DETOUR_MEMBER_CALL(CTFWeaponBaseGrenadeProj_Explode)(pTrace, bitsDamageType);
	}

	DETOUR_DECL_STATIC(void, DispatchParticleEffect, char const *name, Vector vec, QAngle ang, CBaseEntity *entity)
	{
		if (!rc_DoNotOverrideSmoke && strcmp(name, "fluidSmokeExpl_ring_mvm") == 0) {
			name = "hightower_explosion";
		}
		DETOUR_STATIC_CALL(DispatchParticleEffect)(name, vec, ang, entity);
	}

	class CMod_SentryBusterParticleChange : public IMod
	{
	public:
		CMod_SentryBusterParticleChange() : IMod("Etc:Misc:SentryBusterParticleChange")
		{
			
			// Replace buster smoke with something else
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGrenadeProj_Explode, "CTFWeaponBaseGrenadeProj::Explode");
			MOD_ADD_DETOUR_STATIC_PRIORITY(DispatchParticleEffect, "DispatchParticleEffect [overload 3]", HIGH);
		}
	};
	CMod_SentryBusterParticleChange s_SentryBusterParticleChange;

	ConVar sig_etc_sentry_buster_particle_change("sig_etc_sentry_buster_particle_change", "0", FCVAR_NOTIFY,
		"Mod: Replace sentry buster smoke with payload bomb particles",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_SentryBusterParticleChange.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});

	DETOUR_DECL_MEMBER(int, CHeadlessHatman_OnTakeDamage_Alive, const CTakeDamageInfo& info)
	{
		auto npc = reinterpret_cast<CHeadlessHatman *>(this);
		float healthPercentage = (float)npc->GetHealth() / (float)npc->GetMaxHealth();

		if (g_pMonsterResource.GetRef() != nullptr)
		{
			if (healthPercentage <= 0.0f)
			{
				g_pMonsterResource->m_iBossHealthPercentageByte = 0;
			}
			else
			{
				g_pMonsterResource->m_iBossHealthPercentageByte = (int) (healthPercentage * 255.0f);
			}
		}
		return DETOUR_MEMBER_CALL(CHeadlessHatman_OnTakeDamage_Alive)(info);
	}

	DETOUR_DECL_MEMBER(void, CHeadlessHatman_Spawn)
	{
		auto npc = reinterpret_cast<CHeadlessHatman *>(this);
		
		if (g_pMonsterResource.GetRef() != nullptr)
		{
			g_pMonsterResource->m_iBossHealthPercentageByte = 255;
		}
		DETOUR_MEMBER_CALL(CHeadlessHatman_Spawn)();
	}

	DETOUR_DECL_MEMBER(void, CHeadlessHatman_D2)
	{
		auto npc = reinterpret_cast<CHeadlessHatman *>(this);
		if (g_pMonsterResource.GetRef() != nullptr && g_pMonsterResource)
		{
			g_pMonsterResource->m_iBossHealthPercentageByte = 0;
		}
		DETOUR_MEMBER_CALL(CHeadlessHatman_D2)();
	}

	class CMod_HHHHealthBar : public IMod
	{
	public:
		CMod_HHHHealthBar() : IMod("Etc:Misc:HHHHealthBar")
		{
			// Allow HHH to have a healthbar
			MOD_ADD_DETOUR_MEMBER(CHeadlessHatman_OnTakeDamage_Alive, "CHeadlessHatman::OnTakeDamage_Alive");
			MOD_ADD_DETOUR_MEMBER(CHeadlessHatman_Spawn,           "CHeadlessHatman::Spawn");
			MOD_ADD_DETOUR_MEMBER(CHeadlessHatman_D2,           "CHeadlessHatman [D2]");
		}
	};
	CMod_HHHHealthBar s_HHHHealthBar;

	ConVar sig_etc_hhh_health_bar("sig_etc_hhh_health_bar", "0", FCVAR_NOTIFY,
		"Mod: Display boss health bar on HHH",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_HHHHealthBar.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}