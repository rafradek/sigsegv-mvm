#include "mod.h"
#include "stub/tf_shareddefs.h"
#include "stub/gamerules.h"
#include "stub/usermessages_sv.h"


// THE PROBLEM WE'RE ATTEMPTING TO FIX:
// apparently, if there is an MvM mission that has FixedRespawnWaveTime enabled,
// and it has a relatively high RespawnWaveTime, and a player dies during that mission;
// and then, a different MvM mission is switched to, which has lower respawn times,
// then the long respawn wait from the prior mission will have a latent effect
// which causes that player to have to wait out the prior mission's respawn time
// (at least the first time... not entirely sure about the full mechanics here)
// note that this problem does NOT occur across map changes; only across 

// WHAT WE THINK IS GOING ON:
// the game code for initializing/loading/restarting/etc MvM missions SHOULD be
// re-initializing some stuff in the gamerules class related to respawn waves,
// but due to an oversight by the developers, this isn't actually done
// and presumably it doesn't affect PvP since they are more "map-at-a-time" based,
// and the gamerules are fully reset across map loads

// LOOK INTO THIS:
// doesn't PvP have a vote to restart the game?
// does that reset all the respawn wave stuff?
// could it simply be that we need to replicate that code in the MvM-specific
// mission initialization/load/restart/etc function(s)?

// UNFORTUNATELY, THIS FIX IN ITS CURRENT FORM DOES NOT ACTUALLY WORK!


namespace Mod::MvM::Respawn_Wave_Fix
{
	static void PrintToChatAll(const char *str)
	{
		int msg_type = usermessages->LookupUserMessage("SayText2");
		if (msg_type == -1) return;
		
		CReliableBroadcastRecipientFilter filter;
		
		bf_write *msg = engine->UserMessageBegin(&filter, msg_type);
		if (msg == nullptr) return;
		
		msg->WriteByte(0x00);
		msg->WriteByte(0x00);
		msg->WriteString(str);
		
		engine->MessageEnd();
    }
	
	
	void PrintDebugInfo()
	{
		PrintToChatAll(CFmtStr("<<DEBUG>> [NOW: %7.1f] m_flOriginalTeamRespawnWaveTime[TF_TEAM_RED]: %7.1f\n", gpGlobals->curtime, TFGameRules()->m_flOriginalTeamRespawnWaveTime[TF_TEAM_RED]));
		PrintToChatAll(CFmtStr("<<DEBUG>> [NOW: %7.1f] m_TeamRespawnWaveTimes         [TF_TEAM_RED]: %7.1f\n", gpGlobals->curtime, TFGameRules()->m_TeamRespawnWaveTimes         [TF_TEAM_RED]));
		PrintToChatAll(CFmtStr("<<DEBUG>> [NOW: %7.1f] m_flNextRespawnWave            [TF_TEAM_RED]: %7.1f\n", gpGlobals->curtime, TFGameRules()->m_flNextRespawnWave            [TF_TEAM_RED]));
	}
	
	
	void ResetOriginalTeamRespawnWaveTimeArray()
	{
		// TODO: remove this before deploying for real
		PrintDebugInfo();
		
		for (int i = 0; i < MAX_TEAMS; ++i) {
			TFGameRules()->m_flOriginalTeamRespawnWaveTime[i] = -1.0f;
		}
	}
	
	
	CON_COMMAND(sig_mvm_respawn_wave_fix_reset, "Mod: manually force-reset g_pGameRules->m_flOriginalTeamRespawnWaveTime[] array")
	{
		ResetOriginalTeamRespawnWaveTimeArray();
	}
	
	CON_COMMAND(sig_mvm_respawn_wave_fix_debug, "Mod: print information for debugging")
	{
		PrintDebugInfo();
	}
	
	
	DETOUR_DECL_MEMBER(bool, CPopulationManager_Initialize)
	{
		bool success = DETOUR_MEMBER_CALL(CPopulationManager_Initialize)();
		
		if (success) {
			ResetOriginalTeamRespawnWaveTimeArray();
		}
		
		return success;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:Respawn_Wave_Fix")
		{
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_Initialize, "CPopulationManager::Initialize");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_respawn_wave_fix", "0", FCVAR_NOTIFY,
		"Mod: fix long respawn waves persisting across MvM mission loads",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
