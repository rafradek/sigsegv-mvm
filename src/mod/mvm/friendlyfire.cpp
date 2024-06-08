#include "mod.h"
#include "stub/gamerules.h"
#include "stub/tfplayer.h"
#include "stub/tf_shareddefs.h"


namespace Mod::MvM::FriendlyFire
{
	ConVar cvar_lagcompensate("sig_mvm_friendlyfire_lagcompensate", "1", FCVAR_NOTIFY,
		"Mod: enable lag compensation for friendly fire mode");
	
	
	void ConVarEnableChanged(IConVar *pConVar, const char *pOldValue, float flOldValue);
	ConVar cvar_enable("sig_mvm_friendlyfire", "0", FCVAR_NOTIFY,
		"Mod: enable friendly fire for red team in MvM mode (0: off; 1: only before/after waves; 2: always on)", &ConVarEnableChanged);
	
	
	bool InMvMFriendlyFireTime()
	{
		switch (cvar_enable.GetInt()) {
		case 1:  return (TFGameRules()->State_Get() != GR_STATE_RND_RUNNING);
		case 2:  return true;
		default: return false;
		}
	}
	
	
	DETOUR_DECL_MEMBER(bool, CTFGameRules_FPlayerCanTakeDamage, CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo& info)
	{
		if (pPlayer != nullptr && pAttacker != nullptr && TFGameRules()->IsMannVsMachineMode() &&
			pPlayer->GetTeamNumber() == TF_TEAM_RED && pAttacker->GetTeamNumber() == TF_TEAM_RED &&
			InMvMFriendlyFireTime()) {
			return true;
		}
		
		return DETOUR_MEMBER_CALL(pPlayer, pAttacker, info);
	}
	
	DETOUR_DECL_MEMBER(bool, CTFPlayer_WantsLagCompensationOnEntity, const CBasePlayer *pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		/* a bit hacky: turn on lag compensation for EVERYTHING for FF mode */
		if (cvar_lagcompensate.GetBool() && TFGameRules()->IsMannVsMachineMode() && player->GetTeamNumber() == TF_TEAM_RED && InMvMFriendlyFireTime()) {
			return true;
		}
		
		return DETOUR_MEMBER_CALL(pPlayer, pCmd, pEntityTransmitBits);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:FriendlyFire")
		{
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_FPlayerCanTakeDamage,      "CTFGameRules::FPlayerCanTakeDamage");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_WantsLagCompensationOnEntity, "CTFPlayer::WantsLagCompensationOnEntity");
		}
	};
	CMod s_Mod;
	
	
	void ConVarEnableChanged(IConVar *pConVar, const char *pOldValue, float flOldValue)
	{
		ConVarRef var(pConVar);
		s_Mod.Toggle(var.GetBool());
	};
}
