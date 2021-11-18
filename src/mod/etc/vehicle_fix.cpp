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
		vehicle->SetNextThink(gpGlobals->curtime+0.01, "CarThink");
	}
    
    DETOUR_DECL_MEMBER(void, CPropVehicleDriveable_Spawn)
	{
		DETOUR_MEMBER_CALL(CPropVehicleDriveable_Spawn)();
		THINK_FUNC_SET(reinterpret_cast<CBaseEntity *>(this), CarThink, gpGlobals->curtime + 0.01);
	}

	DETOUR_DECL_MEMBER(void, CPlayerMove_SetupMove, CBasePlayer *player, CUserCmd *ucmd, void *pHelper, void *move)
	{
		if (player->m_hVehicle != nullptr) {
			CBaseEntity *entVehicle = player->m_hVehicle;
			auto vehicle = rtti_cast<CPropVehicleDriveable *>(entVehicle);
			if (vehicle != nullptr) {
				CBaseServerVehicle *serverVehicle = vehicle->m_pServerVehicle;
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

				if (vehicleentity != nullptr) {
					variant_t variant;
					vehicleentity->AcceptInput("Use", player, player, variant, USE_ON);
					return nullptr;
				}
			}

			
		}
		return DETOUR_MEMBER_CALL(CMultiplayRules_VoiceCommand)(player, menu, item);
	}

	DETOUR_DECL_MEMBER(bool, CBasePlayer_GetInVehicle, CBaseServerVehicle *vehicle, int mode)
	{
		auto player = reinterpret_cast<CBasePlayer *>(this);
		auto ret = DETOUR_MEMBER_CALL(CBasePlayer_GetInVehicle)(vehicle, mode);
		if (ret) {
			player->m_Local->m_bDrawViewmodel = false;
			if (player->m_hVehicle != nullptr) {
				player->m_hVehicle->SetTeamNumber(player->GetTeamNumber());
			}
		}
		SendConVarValue(ENTINDEX(player), "sv_client_predict", "0");

		return ret;
	}

	DETOUR_DECL_MEMBER(void, CBasePlayer_LeaveVehicle, Vector &pos, QAngle &angles)
	{
		auto player = reinterpret_cast<CBasePlayer *>(this);
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

	VHOOK_DECL(int, CPropVehicleDriveable_OnTakeDamage, const CTakeDamageInfo &info)
	{
		auto vehicle = reinterpret_cast<CPropVehicleDriveable *>(this);
		if (vehicle->m_hPlayer != nullptr) {
			return vehicle->m_hPlayer->TakeDamage(info);
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
	
	
	/* by way of incredibly annoying persistent requests from Hell-met,
	 * I've acquiesced and made this mod convar non-notifying (sigh) */
	ConVar cvar_enable("sig_etc_vehicle_fix", "0", /*FCVAR_NOTIFY*/FCVAR_NONE,
		"Etc: fix prop_vehicle_driveable",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}