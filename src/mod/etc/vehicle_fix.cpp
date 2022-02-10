#include "mod.h"
#include "stub/econ.h"
#include "stub/extraentitydata.h"
#include "stub/projectiles.h"
#include "stub/tfplayer.h"
#include "stub/tfbot.h"
#include "stub/tfweaponbase.h"
#include "stub/objects.h"
#include "stub/misc.h"
#include "stub/gamerules.h"
#include "stub/strings.h"
#include "stub/server.h"
#include "util/admin.h"
#include "util/clientmsg.h"
#include "util/misc.h"
#include "util/iterate.h"
#include <vphysics/vehicles.h>

class IPredictionSystem
{
public:
	void *vtable;
	IPredictionSystem	*m_pNextSystem;
	int				m_bSuppressEvent;
	CBaseEntity			*m_pSuppressHost;

	int					m_nStatusPushed;
};

class CTEPlayerAnimEvent {};

GlobalThunk<IPredictionSystem> g_RecipientFilterPredictionSystem("g_RecipientFilterPredictionSystem");

namespace Mod::Util::Vehicle_Fix
{	

	THINK_FUNC_DECL(CarThink) {
		auto vehicle = reinterpret_cast<CPropVehicleDriveable *>(this);
		int sequence = vehicle->m_nSequence;
		bool sequenceFinished = vehicle->m_bSequenceFinished;
		bool enterAnimOn = vehicle->m_bEnterAnimOn;
		bool exitAnimOn = vehicle->m_bExitAnimOn;

		vehicle->StudioFrameAdvance();
			
		if ((sequence == 0 || sequenceFinished) && (enterAnimOn || exitAnimOn)) {
			if (enterAnimOn)
			{
				variant_t variant;
				vehicle->AcceptInput("TurnOn", vehicle, vehicle, variant, -1);
			}
			
			CBaseServerVehicle *serverVehicle = vehicle->m_pServerVehicle;
			serverVehicle->HandleEntryExitFinish(exitAnimOn, true);
		}
		CBasePlayer *player = vehicle->m_hPlayer;
		if (player != nullptr) {
			vehicle->m_hPhysicsAttacker = player;
			vehicle->m_flLastPhysicsInfluenceTime = gpGlobals->curtime;
			auto angles = player->GetAbsAngles();
		}
		vehicle->SetNextThink(gpGlobals->curtime+0.01, "CarThink");
	}

	THINK_FUNC_DECL(CarThinkPlayerAnim) {
		auto vehicle = reinterpret_cast<CPropVehicleDriveable *>(this);
		CTFPlayer *player = ToTFPlayer(vehicle->m_hPlayer);
		if (player != nullptr && vehicle->GetCustomVariableFloat<"playerdoanim">(1.0f) ) {
			player->PlaySpecificSequence("act_kart_idle");
			vehicle->SetNextThink(gpGlobals->curtime+0.8, "CarThinkPlayerAnim");
		}
	}
    
    DETOUR_DECL_MEMBER(void, CPropVehicleDriveable_Spawn)
	{
		auto vehicle = reinterpret_cast<CPropVehicleDriveable *>(this);

		// Guess vehicle script and type based on model
		if (vehicle->m_vehicleScript == NULL_STRING) {
			if (FStrEq(STRING(vehicle->GetModelName()), "models/buggy.mdl")) {
				vehicle->m_vehicleScript = AllocPooledString("scripts/vehicles/jeep_test.txt");
			}
			else if (FStrEq(STRING(vehicle->GetModelName()), "models/airboat.mdl")) {
				vehicle->m_vehicleScript = AllocPooledString("scripts/vehicles/airboat.txt");
			}
		}

		if (vehicle->m_nVehicleType == 0U) {
			vehicle->m_nVehicleType = vehicle->GetCustomVariableFloat<"vehicletype">();
		}
		if (vehicle->m_nVehicleType == 0U) {
			if (FStrEq(STRING(vehicle->m_vehicleScript.Get()), "scripts/vehicles/jeep_test.txt")) {
				vehicle->m_nVehicleType = 1;
			}
			else if (FStrEq(STRING(vehicle->m_vehicleScript.Get()), "scripts/vehicles/airboat.txt")) {
				vehicle->m_nVehicleType = 8;
			}
		}
		DETOUR_MEMBER_CALL(CPropVehicleDriveable_Spawn)();
		vehicle->SetCollisionGroup(COLLISION_GROUP_PLAYER_MOVEMENT);
		THINK_FUNC_SET(vehicle, CarThink, gpGlobals->curtime + 0.01);
	}

	DETOUR_DECL_MEMBER(void, CPlayerMove_SetupMove, CBasePlayer *player, CUserCmd *ucmd, void *pHelper, void *move)
	{
		if (player->m_hVehicle != nullptr) {
			CBaseEntity *entVehicle = player->m_hVehicle;
			auto vehicle = rtti_cast<CPropVehicleDriveable *>(entVehicle);
			if (vehicle != nullptr) {
				CBaseServerVehicle *serverVehicle = vehicle->m_pServerVehicle;
				if (ucmd->buttons & IN_DUCK && vehicle->GetCustomVariableFloat<"ducktoturbo">(1.0f)) {
					ucmd->buttons |= IN_SPEED;
				}
				serverVehicle->SetupMove(player, ucmd, pHelper, move);
			}
		}
		DETOUR_MEMBER_CALL(CPlayerMove_SetupMove)(player, ucmd, pHelper, move);
	}

	DETOUR_DECL_MEMBER(int, CBaseServerVehicle_GetExitAnimToUse, Vector *vec, bool far)
	{
		return -1;
	}

	DETOUR_DECL_MEMBER(void, CPropVehicleDriveable_Use, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
	{
		if (pActivator != nullptr && pActivator == reinterpret_cast<CPropVehicleDriveable *>(this)->m_hPlayer) return;
		
		DETOUR_MEMBER_CALL(CPropVehicleDriveable_Use)(pActivator, pCaller, useType, value);
	}

	DETOUR_DECL_MEMBER(VoiceCommandMenuItem_t *, CMultiplayRules_VoiceCommand, CBasePlayer* player, int menu, int item)
	{
		if (menu == 0 && item == 0) {
			CBaseEntity *vEnt = player->m_hVehicle;
			CPropVehicleDriveable *vehicleentity = rtti_cast<CPropVehicleDriveable *>(vEnt);

			if (vehicleentity != nullptr) {
				CBaseServerVehicle *vehicle = vehicleentity->m_pServerVehicle;
				vehicle->HandlePassengerExit(player);
				return nullptr;
			}
			else {
				vehicleentity = rtti_cast<CPropVehicleDriveable *>(player->FindUseEntity());

				if (vehicleentity != nullptr && vehicleentity->GetCustomVariableFloat<"callmedictoenter">(1.0f)) {
					variant_t variant;
					vehicleentity->AcceptInput("Use", player, player, variant, USE_ON);
					return nullptr;
				}
			}

			
		}
		return DETOUR_MEMBER_CALL(CMultiplayRules_VoiceCommand)(player, menu, item);
	}

	
	//THINK_FUNC_DECL(EnableCrosshair) {
	//	reinterpret_cast<CTFPlayer *>(this)->m_Local->m_iHideHUD |= HIDEHUD_CROSSHAIR;
	//}
	DETOUR_DECL_MEMBER(bool, CBasePlayer_GetInVehicle, CBaseServerVehicle *vehicle, int mode)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		auto ret = DETOUR_MEMBER_CALL(CBasePlayer_GetInVehicle)(vehicle, mode);
		if (ret) {
			if (player->m_hVehicle != nullptr) {
				player->m_Local->m_bDrawViewmodel = player->m_hVehicle->GetCustomVariableFloat<"allowweapons">();
				if (!player->m_hVehicle->GetCustomVariableFloat<"allowweapons">() && player->GetActiveTFWeapon() != nullptr) {
					player->GetActiveTFWeapon()->SetEffects(player->GetActiveTFWeapon()->GetEffects() | EF_NODRAW);
				}
				player->m_hVehicle->SetOwnerEntity(player);
				player->m_hVehicle->SetTeamNumber(player->GetTeamNumber());
                player->m_hVehicle->SetBlocksLOS(false);
				if (player->m_hVehicle->GetCustomVariableFloat<"showcrosshair">()) {
					player->m_Local->m_iHideHUD &= ~HIDEHUD_CROSSHAIR;
				}
				if (player->m_hVehicle->GetCustomVariableFloat<"playerdoanim">(1.0f)) {
					player->PlaySpecificSequence("taunt_vehicle_allclass_start");
				}
				
				THINK_FUNC_SET(player->m_hVehicle, CarThinkPlayerAnim, gpGlobals->curtime + 2.0);
			}
		}
		SendConVarValue(ENTINDEX(player), "sv_client_predict", "0");
		return ret;
	}

	void ReplaceVehicleProjectileOwner(CBaseEntity *vehicle)
	{
		for (int i = 0; i < IBaseProjectileAutoList::AutoList().Count(); ++i) {
			auto proj = rtti_cast<CBaseProjectile *>(IBaseProjectileAutoList::AutoList()[i]);
			if (proj != nullptr && proj->GetOwnerEntity() == vehicle) {
				auto scorer = rtti_cast<IScorer *>(proj);
				proj->SetOwnerEntity(scorer != nullptr ? scorer->GetScorer() : nullptr);
			}
		}
	}
	DETOUR_DECL_MEMBER(void, CBasePlayer_LeaveVehicle, Vector &pos, QAngle &angles)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		player->DoAnimationEvent( PLAYERANIMEVENT_SPAWN, 0 );
        if (player->m_hVehicle != nullptr) {
			player->m_hVehicle->SetOwnerEntity(nullptr);
            player->m_hVehicle->SetBlocksLOS(true);
			ReplaceVehicleProjectileOwner(player->m_hVehicle);
			
			if (!player->m_hVehicle->GetCustomVariableFloat<"allowweapons">() && player->GetActiveTFWeapon() != nullptr) {
				player->GetActiveTFWeapon()->SetEffects(player->GetActiveTFWeapon()->GetEffects() & ~(EF_NODRAW));
			}
        }
		DETOUR_MEMBER_CALL(CBasePlayer_LeaveVehicle)(pos, angles);
		
		player->m_Local->m_bDrawViewmodel = true;
		static ConVarRef predict("sv_client_predict");
		SendConVarValue(ENTINDEX(player), "sv_client_predict", predict.GetString());
	}

	VHOOK_DECL(unsigned int, CPropVehicleDriveable_PhysicsSolidMaskForEntity)
	{
		auto vehicle = reinterpret_cast<CPropVehicleDriveable *>(this);
		auto mask = VHOOK_CALL(CBaseEntity_PhysicsSolidMaskForEntity)() | CONTENTS_PLAYERCLIP;
		if (vehicle->GetTeamNumber() == TF_TEAM_BLUE)
			mask |= CONTENTS_REDTEAM;
		else if (vehicle->GetTeamNumber() == TF_TEAM_RED)
			mask |= CONTENTS_BLUETEAM;

		return mask;
	}

	RefCount rc_CPropVehicleDriveable_OnTakeDamage;
	VHOOK_DECL(int, CPropVehicleDriveable_OnTakeDamage, const CTakeDamageInfo &info)
	{
		SCOPED_INCREMENT(rc_CPropVehicleDriveable_OnTakeDamage);
		auto vehicle = reinterpret_cast<CPropVehicleDriveable *>(this);
		if (vehicle->m_hPlayer != nullptr && info.GetAttacker() != nullptr && info.GetAttacker()->GetTeamNumber() == vehicle->GetTeamNumber())
		{
			if (info.GetAttacker() != vehicle->m_hPlayer) {
				return 0;
			}
		}
		if (vehicle->m_hPlayer != nullptr && !(info.GetDamageType() & DMG_CRUSH) && (info.GetDamageType() & (DMG_BULLET | DMG_BUCKSHOT | DMG_MELEE)) && vehicle->GetCustomVariableFloat<"passbulletdamage">(1.0f)) {
			CTakeDamageInfo info2 = info;
			info2.SetDamage(info2.GetDamage() * vehicle->GetCustomVariableFloat<"passbulletdamage">(1.0f));
			return vehicle->m_hPlayer->TakeDamage(info2);
		}
		return VHOOK_CALL(CPropVehicleDriveable_OnTakeDamage)(info);

	}

	VHOOK_DECL(void, CPropVehicleDriveable_UpdateOnRemove)
	{
		auto vehicle = reinterpret_cast<CPropVehicleDriveable *>(this);
		if (vehicle->m_hPlayer != nullptr) {
			vehicle->m_hPlayer->LeaveVehicle();
		}
		return VHOOK_CALL(CPropVehicleDriveable_UpdateOnRemove)();
	}

    DETOUR_DECL_MEMBER(void, CTFPlayer_ForceRespawn)
	{
        auto player = reinterpret_cast<CTFPlayer *>(this);
        if (player->m_hVehicle != nullptr) {
            player->LeaveVehicle();
        }
        return DETOUR_MEMBER_CALL(CTFPlayer_ForceRespawn)();
    }

	DETOUR_DECL_MEMBER(bool, CBaseServerVehicle_IsPassengerVisible, int role)
	{
		auto vehicle = reinterpret_cast<CBaseServerVehicle *>(this);
		return !vehicle->GetVehicleEnt()->GetCustomVariableFloat<"hidedriver">();
    }

	DETOUR_DECL_MEMBER(bool, CBaseServerVehicle_IsPassengerUsingStandardWeapons, int role)
	{
		auto vehicle = reinterpret_cast<CBaseServerVehicle *>(this);
		return vehicle->GetVehicleEnt()->GetCustomVariableFloat<"allowweapons">();
    }

	DETOUR_DECL_MEMBER(void, CTFWeaponBaseGun_UpdatePunchAngles, CTFPlayer *player)
	{
		if (player->m_hVehicle == nullptr)
			DETOUR_MEMBER_CALL(CTFWeaponBaseGun_UpdatePunchAngles)(player);
	}

	// Prevent driver's rockets from colliding with the vehicle 
	void SetOwnerToVehicle(CBaseProjectile *projectile)
	{
		auto player = ToTFPlayer(projectile->GetOwnerEntity());
		if (player != nullptr && player->m_hVehicle != nullptr && rtti_cast<IScorer *>(projectile) != nullptr) {
			projectile->SetOwnerEntity(player->m_hVehicle);
		}
	}
	DETOUR_DECL_MEMBER(void, CTFBaseProjectile_Spawn)
	{
		DETOUR_MEMBER_CALL(CTFBaseProjectile_Spawn)();
		SetOwnerToVehicle(reinterpret_cast<CBaseProjectile *>(this));
	}

	DETOUR_DECL_MEMBER(void, CTFProjectile_Arrow_Spawn)
	{
		DETOUR_MEMBER_CALL(CTFProjectile_Arrow_Spawn)();
		SetOwnerToVehicle(reinterpret_cast<CBaseProjectile *>(this));
	}

	bool eyeangles_reentrancy_guard = false;
	DETOUR_DECL_MEMBER(QAngle &, CBasePlayer_EyeAngles)
	{
		auto player = reinterpret_cast<CBasePlayer *>(this);
		if (player->m_hVehicle != nullptr && !eyeangles_reentrancy_guard) {
			Vector fwd;
			eyeangles_reentrancy_guard =true;
			player->EyeVectors(&fwd);
			eyeangles_reentrancy_guard =false;
			static QAngle angles;
			VectorAngles(fwd, angles);
			angles.x = AngleNormalize(angles.x);
			return angles;
			//DevMsg("Angles %f %f %f %f\n", angles.x, player->m_hVehicle->GetAbsAngles().x, player->m_hVehicle->GetAbsAngles().y, player->m_hVehicle->GetAbsAngles().z);
			//angles.x += player->m_hVehicle->GetAbsAngles().z + 0.5;
		}
		return DETOUR_MEMBER_CALL(CBasePlayer_EyeAngles)();
	}

	/*DETOUR_DECL_MEMBER(unsigned int, CTFBaseProjectile_PhysicsSolidMaskForEntity)
	{
		auto owner = ToTFPlayer(reinterpret_cast<CTFBaseProjectile *>(this)->GetOwnerEntity());
		if (owner != nullptr && owner->m_hVehicle != nullptr) {
			return DETOUR_MEMBER_CALL(CTFBaseProjectile_PhysicsSolidMaskForEntity)() | CON;
		}
    }*/
	
	RefCount rc_CTFWeaponBase_PrimaryAttack;
	DETOUR_DECL_MEMBER(void, CTFWeaponBase_PrimaryAttack)
	{
		auto player = reinterpret_cast<CTFWeaponBase *>(this)->GetTFPlayerOwner();
		auto oldHost = g_RecipientFilterPredictionSystem.GetRef().m_pSuppressHost;
		if (player != nullptr && player->m_hVehicle != nullptr) {
			g_RecipientFilterPredictionSystem.GetRef().m_pSuppressHost = nullptr;
		}
		DETOUR_MEMBER_CALL(CTFWeaponBase_PrimaryAttack)();
		g_RecipientFilterPredictionSystem.GetRef().m_pSuppressHost = oldHost;
	}
	
	DETOUR_DECL_MEMBER(void, CPropVehicleDriveable_TraceAttack, const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, void *pAccumulator)
	{
		DevMsg("Has trace attack\n");
		CTakeDamageInfo info2 = info;
		auto vehicle = reinterpret_cast<CPropVehicleDriveable *>(this);
		float strength = vehicle->GetCustomVariableFloat<"meleeforce">(1.0f);
		bool meleegetin = vehicle->GetCustomVariableFloat<"meleetoenter">(1.0f);
		if (info2.GetDamageType() & DMG_MELEE && info2.GetAttacker() != nullptr && info2.GetAttacker()->IsPlayer() && vehicle->m_hPlayer == nullptr) {
			if (meleegetin) {
				CBaseServerVehicle *serverVehicle = vehicle->m_pServerVehicle;
				serverVehicle->HandlePassengerEntry(ToTFPlayer(info2.GetAttacker()), false);
				if (vehicle->m_hPlayer != nullptr)
					return;
			}
			if (strength != 0.0f && info2.GetAttacker()->GetTeamNumber() == vehicle->GetTeamNumber()) {
				Vector force = info2.GetDamageForce();
				force.z = 250 * vehicle->VPhysicsGetObject()->GetMass() * strength;
				force.x *= 5;
				force.y *= 5;
				info2.SetDamageForce(force);
			}
		}
		//info2.SetDamageType(info2.GetDamageType() | DMG_PHYSGUN);
		DETOUR_MEMBER_CALL(CPropVehicleDriveable_TraceAttack)(info2, vecDir, ptr, pAccumulator);

	}

	DETOUR_DECL_MEMBER(void, CPropVehicleDriveable_DriveVehicle, float flFrameTime, CUserCmd *ucmd, int iButtonsDown, int iButtonsReleased)
	{
		DETOUR_MEMBER_CALL(CPropVehicleDriveable_DriveVehicle)(flFrameTime, ucmd, iButtonsDown, iButtonsReleased);
		auto vehicle = reinterpret_cast<CPropVehicleDriveable *>(this);
		variant_t variant;
		if (iButtonsReleased & IN_ATTACK) {
			vehicle->FireCustomOutput<"unpressedattack">(vehicle->m_hPlayer, vehicle, variant);
		}
		if (iButtonsReleased & IN_ATTACK2) {
			vehicle->FireCustomOutput<"unpressedattack2">(vehicle->m_hPlayer, vehicle, variant);
		}
		if (iButtonsReleased & IN_FORWARD) {
			vehicle->FireCustomOutput<"unpressedforward">(vehicle->m_hPlayer, vehicle, variant);
		}
		if (iButtonsDown & IN_FORWARD) {
			vehicle->FireCustomOutput<"pressedforward">(vehicle->m_hPlayer, vehicle, variant);
		}
		if (iButtonsReleased & IN_BACK) {
			vehicle->FireCustomOutput<"unpressedback">(vehicle->m_hPlayer, vehicle, variant);
		}
		if (iButtonsDown & IN_BACK) {
			vehicle->FireCustomOutput<"pressedback">(vehicle->m_hPlayer, vehicle, variant);
		}
		if (iButtonsReleased & IN_MOVELEFT) {
			vehicle->FireCustomOutput<"unpressedmoveleft">(vehicle->m_hPlayer, vehicle, variant);
		}
		if (iButtonsDown & IN_MOVELEFT) {
			vehicle->FireCustomOutput<"pressedmoveleft">(vehicle->m_hPlayer, vehicle, variant);
		}
		if (iButtonsReleased & IN_MOVERIGHT) {
			vehicle->FireCustomOutput<"unpressedmoveright">(vehicle->m_hPlayer, vehicle, variant);
		}
		if (iButtonsDown & IN_MOVERIGHT) {
			vehicle->FireCustomOutput<"pressedmoveright">(vehicle->m_hPlayer, vehicle, variant);
		}
	}

	DETOUR_DECL_MEMBER(bool, CPropVehicleDriveable_PassengerShouldReceiveDamage, const CTakeDamageInfo &inputInfo)
	{
		auto vehicle = reinterpret_cast<CPropVehicleDriveable *>(this);
		return vehicle->GetCustomVariableFloat<"allowpassengerdamage">(1.0f);
	}
	
	struct gamevcollisionevent_t : public vcollisionevent_t
	{
		Vector			preVelocity[2];
		Vector			postVelocity[2];
		AngularImpulse	preAngularVelocity[2];
		CBaseEntity		*pEntities[2];
	};

	DETOUR_DECL_STATIC(float, CalculatePhysicsImpactDamage, int index, gamevcollisionevent_t *pEvent, void *table, float energyScale, bool allowStaticDamage, int &damageTypeOut, bool bDamageFromHeldObjects)
	{
		auto damage = DETOUR_STATIC_CALL(CalculatePhysicsImpactDamage)(index, pEvent, table, energyScale, allowStaticDamage, damageTypeOut, bDamageFromHeldObjects);
		if (pEvent->pEntities[0] != nullptr) {
			damage *= pEvent->pEntities[0]->GetCustomVariableFloat<"impactdamagemult">(1.0f);
		}
		if (pEvent->pEntities[1] != nullptr) {
			damage *= pEvent->pEntities[1]->GetCustomVariableFloat<"impactdamagemult">(1.0f);
		}
		return damage;
	}

	DETOUR_DECL_MEMBER(float, CBaseCombatCharacter_CalculatePhysicsStressDamage, void *stress, IPhysicsObject *obj)
	{
		auto character = reinterpret_cast<CBaseCombatCharacter *>(this);
		auto ent = reinterpret_cast<CBaseEntity *>(obj->GetGameData());
		if (character->IsPlayer()/* && character->VPhysicsGetObject() != nullptr && !character->VPhysicsGetObject()->IsMoveable()*/) {
			return 0;
		}
		return DETOUR_MEMBER_CALL(CBaseCombatCharacter_CalculatePhysicsStressDamage)(stress, obj);
	}

	THINK_FUNC_DECL(ApplyImmobile)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (player->VPhysicsGetObject() != nullptr)
			player->VPhysicsGetObject()->EnableMotion(!player->IsMiniBoss());
	}

	DETOUR_DECL_MEMBER(void, CTFGameRules_OnPlayerSpawned, CTFPlayer *player)
	{
		THINK_FUNC_SET(player, ApplyImmobile, gpGlobals->curtime+0.01f);
		DETOUR_MEMBER_CALL(CTFGameRules_OnPlayerSpawned)(player);
	}

	DETOUR_DECL_STATIC(void, SV_ComputeClientPacks, int clientCount,  void **clients, void *snapshot)
	{
		static float angpre[34];
		ForEachTFPlayer([](CTFPlayer *player) {
			if (player->m_hVehicle != nullptr && player->m_hVehicle->GetCustomVariableFloat<"playermodellookforward">(1.0f)) {
				QAngle ang = player->m_angEyeAngles;
				angpre[ENTINDEX(player)] = ang.y;
				ang.y = player->m_hVehicle->GetAbsAngles().y + 90;
				player->m_angEyeAngles = ang;

			}
		}); 
		DETOUR_STATIC_CALL(SV_ComputeClientPacks)(clientCount, clients, snapshot);
		ForEachTFPlayer([](CTFPlayer *player) {
			if (player->m_hVehicle != nullptr && player->m_hVehicle->GetCustomVariableFloat<"playermodellookforward">(1.0f)) {
				QAngle ang = player->m_angEyeAngles;
				ang.y = angpre[ENTINDEX(player)];
				player->m_angEyeAngles = ang;

			}
		}); 
	}
	
	DETOUR_DECL_MEMBER(void, CBaseCombatWeapon_WeaponSound, int index, float soundtime) 
	{
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		CTFPlayer *owner = weapon->GetTFPlayerOwner();
		auto oldHost = g_RecipientFilterPredictionSystem.GetRef().m_pSuppressHost;
		if (owner != nullptr && owner->m_hVehicle != nullptr) {
			g_RecipientFilterPredictionSystem.GetRef().m_pSuppressHost = nullptr;
		}
		DETOUR_MEMBER_CALL(CBaseCombatWeapon_WeaponSound)(index, soundtime);
		g_RecipientFilterPredictionSystem.GetRef().m_pSuppressHost = oldHost;
	}

	DETOUR_DECL_MEMBER(void, CTFProjectile_EnergyRing_ProjectileTouch, CBaseEntity *pOther)
	{
		if (rtti_cast<CPropVehicleDriveable *>(pOther) != nullptr) {
			return;
		}
		DETOUR_MEMBER_CALL(CTFProjectile_EnergyRing_ProjectileTouch)(pOther);
	}

	VHOOK_DECL(bool, CPropVehicleDriveable_IsCombatItem)
	{
		return reinterpret_cast<CPropVehicleDriveable *>(this)->GetTeamNumber() != 0 && reinterpret_cast<CPropVehicleDriveable *>(this)->m_hPlayer != nullptr;
	}

    class CMod : public IMod, public IModCallbackListener
	{
	public:
		CMod() : IMod("Util:Vehicle_Fix")
		{
			MOD_ADD_DETOUR_MEMBER(CPropVehicleDriveable_Spawn, "CPropVehicleDriveable::Spawn");
			MOD_ADD_DETOUR_MEMBER(CPlayerMove_SetupMove, "CPlayerMove::SetupMove");
			MOD_ADD_DETOUR_MEMBER(CBaseServerVehicle_GetExitAnimToUse, "CBaseServerVehicle::GetExitAnimToUse");
			MOD_ADD_DETOUR_MEMBER(CPropVehicleDriveable_Use, "CPropVehicleDriveable::Use");
			MOD_ADD_DETOUR_MEMBER(CMultiplayRules_VoiceCommand, "CMultiplayRules::VoiceCommand");
			MOD_ADD_DETOUR_MEMBER(CBasePlayer_GetInVehicle, "CBasePlayer::GetInVehicle");
			MOD_ADD_DETOUR_MEMBER(CBasePlayer_LeaveVehicle, "CBasePlayer::LeaveVehicle");
			MOD_ADD_VHOOK(CPropVehicleDriveable_PhysicsSolidMaskForEntity, TypeName<CPropVehicleDriveable>(), "CBaseEntity::PhysicsSolidMaskForEntity");
			MOD_ADD_VHOOK(CPropVehicleDriveable_OnTakeDamage, TypeName<CPropVehicleDriveable>(), "CBaseEntity::OnTakeDamage");
			MOD_ADD_VHOOK(CPropVehicleDriveable_UpdateOnRemove, TypeName<CPropVehicleDriveable>(), "CBaseEntity::UpdateOnRemove");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ForceRespawn, "CTFPlayer::ForceRespawn");
			MOD_ADD_DETOUR_MEMBER(CBaseServerVehicle_IsPassengerVisible, "CBaseServerVehicle::IsPassengerVisible");
			MOD_ADD_DETOUR_MEMBER(CBaseServerVehicle_IsPassengerUsingStandardWeapons, "CBaseServerVehicle::IsPassengerUsingStandardWeapons");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_UpdatePunchAngles, "CTFWeaponBaseGun::UpdatePunchAngles");
			//MOD_ADD_DETOUR_MEMBER(CTFBaseProjectile_Spawn, "CTFBaseProjectile::Spawn");
			//MOD_ADD_DETOUR_MEMBER(CTFProjectile_Arrow_Spawn, "CTFProjectile_Arrow::Spawn");
			MOD_ADD_DETOUR_MEMBER(CBasePlayer_EyeAngles, "CBasePlayer::EyeAngles");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_PrimaryAttack, "CTFWeaponBaseGun::PrimaryAttack");
			MOD_ADD_DETOUR_MEMBER(CPropVehicleDriveable_TraceAttack, "CPropVehicleDriveable::TraceAttack");
			MOD_ADD_DETOUR_MEMBER(CPropVehicleDriveable_DriveVehicle, "CPropVehicleDriveable::DriveVehicle");
			MOD_ADD_DETOUR_MEMBER(CPropVehicleDriveable_PassengerShouldReceiveDamage, "CPropVehicleDriveable::PassengerShouldReceiveDamage");
			MOD_ADD_DETOUR_STATIC(CalculatePhysicsImpactDamage, "CalculatePhysicsImpactDamage");
			MOD_ADD_DETOUR_MEMBER(CBaseCombatCharacter_CalculatePhysicsStressDamage, "CBaseCombatCharacter::CalculatePhysicsStressDamage");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_OnPlayerSpawned, "CTFGameRules::OnPlayerSpawned");
			MOD_ADD_DETOUR_STATIC(SV_ComputeClientPacks, "SV_ComputeClientPacks");
			MOD_ADD_DETOUR_MEMBER(CBaseCombatWeapon_WeaponSound, "CBaseCombatWeapon::WeaponSound");
			//MOD_ADD_DETOUR_MEMBER(CTFProjectile_EnergyRing_ProjectileTouch, "CTFProjectile_EnergyRing::ProjectileTouch");
			MOD_ADD_VHOOK(CPropVehicleDriveable_IsCombatItem, TypeName<CPropVehicleDriveable>(), "CBaseEntity::IsCombatItem");
		}

    virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

        virtual void LevelInitPreEntity() override
		{
            if (soundemitterbase != nullptr) {
                soundemitterbase->AddSoundOverrides("scripts/game_sounds_vehicles.txt", true);
            }
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_etc_vehicle_fix", "0", FCVAR_NOTIFY,
		"Etc: fix prop_vehicle_driveable",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}