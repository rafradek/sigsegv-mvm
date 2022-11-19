#include "mod.h"
#include "stub/baseentity.h"
#include "stub/tfweaponbase.h"
#include "stub/projectiles.h"
#include "stub/objects.h"
#include "stub/entities.h"
#include "stub/nextbot_cc.h"
#include "stub/gamerules.h"
#include "stub/nav.h"
#include "util/backtrace.h"
#include "util/clientmsg.h"
#include "util/misc.h"
#include "tier1/CommandBuffer.h"


namespace Mod::Etc::Misc
{

	DETOUR_DECL_MEMBER(bool, CTFProjectile_Rocket_IsDeflectable)
	{
		auto ent = reinterpret_cast<CTFProjectile_Rocket *>(this);

		if (strcmp(ent->GetClassname(), "tf_projectile_balloffire") == 0) {
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
			CBaseObject *obj = player->GetObject(i);
			if (obj != nullptr && obj->m_bDisposableBuilding) {
				if (!firstminisentry)
					iElement++;
				firstminisentry = false;
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
			CBaseObject *obj = player->GetObject(i);
			if (obj != nullptr && obj->m_bDisposableBuilding) {
				if (!firstminisentry)
					count--;
				firstminisentry = false;
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

	bool ActivateShield(CTFPlayer *player)
	{
		for (int i = 0; i < player->GetNumWearables(); i++) {
			auto shield = rtti_cast<CTFWearableDemoShield *>(player->GetWearable(i));
			if (shield != nullptr && !shield->m_bDisguiseWearable) {
				shield->DoSpecialAction(player);
				return true;
			}
		}
		return false; 
	}

	DETOUR_DECL_MEMBER(bool, CTFPlayer_DoClassSpecialSkill)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		auto result = DETOUR_MEMBER_CALL(CTFPlayer_DoClassSpecialSkill)();

		if (!player->IsAlive())
			return result;

		if (player->m_Shared->InCond(TF_COND_HALLOWEEN_KART))
			return result;

		if (player->m_Shared->m_bHasPasstimeBall)
			return result; 

		if (!result && player->GetPlayerClass()->GetClassIndex() != TF_CLASS_DEMOMAN) {
			auto sticky = rtti_cast<CTFPipebombLauncher *>(player->GetEntityForLoadoutSlot(LOADOUT_POSITION_SECONDARY));
			if (sticky != nullptr) {
				sticky->SecondaryAttack();
				result = true;
			}
			else {
				result = ActivateShield(player);
			}
		}

		if (!result && player->GetPlayerClass()->GetClassIndex() != TF_CLASS_ENGINEER && player->GetObjectCount() > 0 && player->Weapon_OwnsThisID(TF_WEAPON_BUILDER) != nullptr) {
			result = player->TryToPickupBuilding();
		}
		return result;
	}

	DETOUR_DECL_MEMBER(void, CTFPlayerShared_UpdateChargeMeter)
	{
		auto player = reinterpret_cast<CTFPlayerShared *>(this);

		auto playerClass = player->GetOuter()->GetPlayerClass();
		int classPre = playerClass->GetClassIndex();
		playerClass->SetClassIndex(TF_CLASS_DEMOMAN);
		DETOUR_MEMBER_CALL(CTFPlayerShared_UpdateChargeMeter)();
		playerClass->SetClassIndex(classPre);
	}

	DETOUR_DECL_MEMBER(bool, CTFPlayer_EndClassSpecialSkill)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		auto result = DETOUR_MEMBER_CALL(CTFPlayer_EndClassSpecialSkill)();

		if (result && player->GetPlayerClass()->GetClassIndex() != TF_CLASS_DEMOMAN) {
			int classPre = player->GetPlayerClass()->GetClassIndex();
			player->GetPlayerClass()->SetClassIndex(TF_CLASS_DEMOMAN);
			result = DETOUR_MEMBER_CALL(CTFPlayer_EndClassSpecialSkill)();
			player->GetPlayerClass()->SetClassIndex(classPre);
		}
		return result;
	}

	DETOUR_DECL_MEMBER(float, CTFPlayer_TeamFortress_CalculateMaxSpeed, bool flag)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);

		int classPre = player->GetPlayerClass()->GetClassIndex();
		bool charging = !flag && player->GetPlayerClass()->GetClassIndex() != TF_CLASS_DEMOMAN && player->m_Shared->InCond(TF_COND_SHIELD_CHARGE);
		if (charging) {
			player->GetPlayerClass()->SetClassIndex(TF_CLASS_DEMOMAN);
		}
		float ret = DETOUR_MEMBER_CALL(CTFPlayer_TeamFortress_CalculateMaxSpeed)(flag);
		
		if (player->GetPlayerClass()->GetClassIndex() != TF_CLASS_DEMOMAN) {
			auto sword = rtti_cast<CTFSword *>(player->GetEntityForLoadoutSlot(LOADOUT_POSITION_MELEE));
			if (sword != nullptr) {
				ret *= sword->GetSwordSpeedMod();
			}
		}
		if (charging) {
			player->GetPlayerClass()->SetClassIndex(classPre);
		}

		return ret;
	}

	DETOUR_DECL_MEMBER(int, CTFPlayer_GetMaxHealthForBuffing)
	{
		int ret = DETOUR_MEMBER_CALL(CTFPlayer_GetMaxHealthForBuffing)();
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (player->GetPlayerClass()->GetClassIndex() != TF_CLASS_DEMOMAN) {
			auto sword = rtti_cast<CTFSword *>(player->GetEntityForLoadoutSlot(LOADOUT_POSITION_MELEE));
			if (sword != nullptr) {
				ret += sword->GetSwordHealthMod();
			}
		}
		return ret;
	}

	DETOUR_DECL_MEMBER(void, CTFMinigun_SecondaryAttack)
	{
		auto player = reinterpret_cast<CTFMinigun *>(this)->GetTFPlayerOwner();
		DETOUR_MEMBER_CALL(CTFMinigun_SecondaryAttack)();
		ActivateShield(player);	
	}

	DETOUR_DECL_MEMBER(void, CTFFists_SecondaryAttack)
	{
		auto player = reinterpret_cast<CTFWeaponBaseMelee *>(this)->GetTFPlayerOwner();
		if (!ActivateShield(player)) {
			DETOUR_MEMBER_CALL(CTFFists_SecondaryAttack)();
		}
	}

	DETOUR_DECL_MEMBER(void, CTFFists_Punch)
	{
		auto player = reinterpret_cast<CTFWeaponBaseMelee *>(this)->GetTFPlayerOwner();
		for (int i = 0; i < player->GetNumWearables(); i++) {
			auto shield = rtti_cast<CTFWearableDemoShield *>(player->GetWearable(i));
			if (shield != nullptr) {
				shield->EndSpecialAction(player);
				break;
			}
		}
		DETOUR_MEMBER_CALL(CTFFists_Punch)();
	}

	DETOUR_DECL_STATIC(bool, ClassCanBuild, int classIndex, int type)
	{
		return true;
	}

	DETOUR_DECL_MEMBER(void, CTFWeaponBuilder_SetSubType, int type)
	{
		if ((type < 0 || type >= OBJ_LAST) && type != 255)
			return;
		DETOUR_MEMBER_CALL(CTFWeaponBuilder_SetSubType)(type);
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_ManageBuilderWeapons, TFPlayerClassData_t *pData)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		int classIndex = player->GetPlayerClass()->GetClassIndex();
		// Do not manage builder weapons for other classes than engineer or spy. This way, custom builder items assigned to those classes are not being destroyed
		if (classIndex != TF_CLASS_ENGINEER && classIndex != TF_CLASS_SPY)
			return;

		DETOUR_MEMBER_CALL(CTFPlayer_ManageBuilderWeapons)(pData);
	}

	THINK_FUNC_DECL(SetTypeToSapper)
	{
		auto builder = reinterpret_cast<CTFWeaponBuilder *>(this);
		if (builder != builder->GetTFPlayerOwner()->GetActiveTFWeapon()) {
			builder->SetSubType(3);
			builder->m_iObjectMode = 0;
		}
		builder->SetNextThink(gpGlobals->curtime + 0.5f, "SetTypeToSapper");
	}

	THINK_FUNC_DECL(SetBuildableObjectTypes)
	{
		auto builder = reinterpret_cast<CTFWeaponBuilder *>(this);
		auto kv = builder->GetItem()->GetStaticData()->GetKeyValues()->FindKey("used_by_classes");
		int typesAllowed = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(builder, typesAllowed, allowed_build_types);
		if (typesAllowed == 0) {
			bool isSapper = kv != nullptr && kv->FindKey("spy") != nullptr;
			typesAllowed = isSapper ? 8: 7;
		}
		builder->m_aBuildableObjectTypes.SetIndex((typesAllowed & 1) == 1, 0);
		builder->m_aBuildableObjectTypes.SetIndex((typesAllowed & 2) == 2, 1);
		builder->m_aBuildableObjectTypes.SetIndex((typesAllowed & 4) == 4, 2);
		builder->m_aBuildableObjectTypes.SetIndex((typesAllowed & 8) == 8, 3);
		if (typesAllowed & 8) {
			builder->SetSubType(3);
			builder->m_iObjectMode = 0;
			// Keep resetting the type to sapper mode
			if (typesAllowed != 8) {
				THINK_FUNC_SET(reinterpret_cast<CTFWeaponBuilder *>(this), SetTypeToSapper, gpGlobals->curtime + 0.5f);
			}
		}
	}
	VHOOK_DECL(void, CTFWeaponBuilder_Equip, CBaseCombatCharacter *owner)
	{
		VHOOK_CALL(CTFWeaponBuilder_Equip)(owner);
		THINK_FUNC_SET(reinterpret_cast<CTFWeaponBuilder *>(this), SetBuildableObjectTypes, gpGlobals->curtime);
	}
	VHOOK_DECL(void, CTFWeaponSapper_Equip, CBaseCombatCharacter *owner)
	{
		VHOOK_CALL(CTFWeaponSapper_Equip)(owner);
		THINK_FUNC_SET(reinterpret_cast<CTFWeaponSapper *>(this), SetBuildableObjectTypes, gpGlobals->curtime);
	}

	DETOUR_DECL_MEMBER(bool, CTFPlayer_ClientCommand, const CCommand& args)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		if (FStrEq(args[0], "destroy") && player->GetPlayerClass()->GetClassIndex() != TF_CLASS_ENGINEER) {
			
			int iBuilding = 0;
			int iMode = 0;
			bool bArgsChecked = false;

			// Fixup old binds.
			if ( args.ArgC() == 2 )
			{
				iBuilding = atoi( args[ 1 ] );
				if ( iBuilding == 3 ) // Teleport exit is now a mode.
				{
					iBuilding = 1;
					iMode = 1;
				}
				bArgsChecked = true;
			}
			else if ( args.ArgC() == 3 )
			{
				iBuilding = atoi( args[ 1 ] );
				if (iBuilding == 3) return false; // No destroying sappers
				iMode = atoi( args[ 2 ] );
				bArgsChecked = true;
			}

			if ( bArgsChecked )
			{
				player->DetonateObjectOfType( iBuilding, iMode );
			}
			else
			{
				ClientMsg(player, "Usage: destroy <building> <mode>\n" );
			}
			return true;
		}
		
		return DETOUR_MEMBER_CALL(CTFPlayer_ClientCommand)(args);
	}

	DETOUR_DECL_MEMBER(void, CFuncIllusionary_Spawn)
	{
		auto entity = reinterpret_cast<CBaseEntity *>(this);
		CBaseEntity::PrecacheModel(STRING(entity->GetModelName()));
		DETOUR_MEMBER_CALL(CFuncIllusionary_Spawn)();
	}

    class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Misc")
		{
            // Make dragons fury projectile non reflectable
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Rocket_IsDeflectable, "CTFProjectile_Rocket::IsDeflectable");

			// Make unowned sentry rocket deal damage
			MOD_ADD_DETOUR_MEMBER(CTFBaseRocket_Explode,    "CTFBaseRocket::Explode");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_RadiusDamage, "CTFGameRules::RadiusDamage");

			// Makes penetration arrows not collide with bounding boxes of various entities
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Arrow_ArrowTouch, "CTFProjectile_Arrow::ArrowTouch");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_BallOfFire_RocketTouch, "CTFProjectile_BallOfFire::RocketTouch");

			// Allow to construct disposable sentries by destroying the oldest ones
			MOD_ADD_DETOUR_STATIC(SendProxy_PlayerObjectList,    "SendProxy_PlayerObjectList");
			MOD_ADD_DETOUR_STATIC(SendProxyArrayLength_PlayerObjects,    "SendProxyArrayLength_PlayerObjects");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_StartBuildingObjectOfType, "CTFPlayer::StartBuildingObjectOfType");

			// Drop flag to last bot nav area
			MOD_ADD_DETOUR_MEMBER(CCaptureFlag_Drop, "CCaptureFlag::Drop");

			// Allow HHH to have a healthbar
			MOD_ADD_DETOUR_MEMBER(CHeadlessHatman_OnTakeDamage_Alive, "CHeadlessHatman::OnTakeDamage_Alive");
			MOD_ADD_DETOUR_MEMBER(CHeadlessHatman_Spawn,           "CHeadlessHatman::Spawn");
			MOD_ADD_DETOUR_MEMBER(CHeadlessHatman_D2,           "CHeadlessHatman [D2]");
			
			// Replace buster smoke with something else
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGrenadeProj_Explode, "CTFWeaponBaseGrenadeProj::Explode");
			MOD_ADD_DETOUR_STATIC_PRIORITY(DispatchParticleEffect, "DispatchParticleEffect [overload 3]", HIGH);

			// Allow non demos to use shields and benefit from eyelander heads
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_DoClassSpecialSkill, "CTFPlayer::DoClassSpecialSkill");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_UpdateChargeMeter, "CTFPlayerShared::UpdateChargeMeter");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_EndClassSpecialSkill, "CTFPlayer::EndClassSpecialSkill");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_TeamFortress_CalculateMaxSpeed, "CTFPlayer::TeamFortress_CalculateMaxSpeed");
			//MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetMaxHealthForBuffing, "CTFPlayer::GetMaxHealthForBuffing");
			MOD_ADD_DETOUR_MEMBER(CTFMinigun_SecondaryAttack, "CTFMinigun::SecondaryAttack");
			MOD_ADD_DETOUR_MEMBER(CTFFists_SecondaryAttack, "CTFFists::SecondaryAttack");
			MOD_ADD_DETOUR_MEMBER(CTFFists_Punch, "CTFFists::Punch");

			// Allow other classes to build and destroy buildings
			MOD_ADD_DETOUR_STATIC(ClassCanBuild, "ClassCanBuild");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBuilder_SetSubType, "CTFWeaponBuilder::SetSubType");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ManageBuilderWeapons, "CTFPlayer::ManageBuilderWeapons");
			MOD_ADD_VHOOK(CTFWeaponBuilder_Equip, TypeName<CTFWeaponBuilder>(), "CTFWeaponBase::Equip");
			MOD_ADD_VHOOK(CTFWeaponSapper_Equip, TypeName<CTFWeaponSapper>(), "CTFWeaponBase::Equip");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ClientCommand, "CTFPlayer::ClientCommand");

			// Fix cfuncillusionary crash
			MOD_ADD_DETOUR_MEMBER(CFuncIllusionary_Spawn, "CFuncIllusionary::Spawn");
		}
	};
	CMod s_Mod;

    ConVar cvar_enable("sig_etc_misc", "0", FCVAR_NOTIFY,
		"Mod: Stuff i am lazy to make into separate mods",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}