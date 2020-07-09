#include "mod.h"
#include "stub/tfbot.h"
#include "stub/populators.h"
#include "util/scope.h"


namespace Mod::MvM::Robot_Limit
{
	void CheckForMaxInvadersAndKickExtras(CUtlVector<CTFPlayer *>& mvm_bots);
	
	
	ConVar cvar_override("sig_mvm_robot_limit_override", "22", FCVAR_NOTIFY,
		"Mod: override the max number of MvM robots that are allowed to be spawned at once (normally 22)",
		true, 0, false, 0,
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			/* ensure that slots are cleared up when this convar is decreased */
			CUtlVector<CTFPlayer *> mvm_bots;
			CPopulationManager::CollectMvMBots(&mvm_bots);
			CheckForMaxInvadersAndKickExtras(mvm_bots);
			DevMsg("Changed\n");
		});
	
	ConVar cvar_fix_red("sig_mvm_robot_limit_fix_red", "0", FCVAR_NOTIFY,
		"Mod: fix problems with enforcement of the MvM robot limit when bots are on red team");
	
	
	int GetMvMInvaderLimit() { return cvar_override.GetInt(); }
	
	
	// reimplement the MvM bots-over-quota logic, with some changes:
	// - use the overridden max bot count, rather than a hardcoded 22
	// - add a third pass to the collect-bots-to-kick logic for bots on TF_TEAM_RED
	void CheckForMaxInvadersAndKickExtras(CUtlVector<CTFPlayer *>& mvm_bots)
	{
		if (mvm_bots.Count() < GetMvMInvaderLimit()) return;
		
		extern RefCount rc_CTFBotSpawner_Spawn;
		static ConVarRef tf_populator_debug("tf_populator_debug");
		if (rc_CTFBotSpawner_Spawn > 0 && tf_populator_debug.GetBool()) {
			DevMsg("CTFBotSpawner: %3.2f: *** Can't spawn. Max number invaders already spawned.\n", gpGlobals->curtime);
		}
		
		if (mvm_bots.Count() > GetMvMInvaderLimit()) {
			CUtlVector<CTFPlayer *> bots_to_kick;
			int need_to_kick = (mvm_bots.Count() - GetMvMInvaderLimit());
			DevMsg("Need to kick %d bots\n", need_to_kick);
			
			/* pass 1: nominate bots on TEAM_SPECTATOR to be kicked */
			for (auto bot : mvm_bots) {
				if (need_to_kick <= 0) break;
				
				if (bot->GetTeamNumber() == TEAM_SPECTATOR) {
					bots_to_kick.AddToTail(bot);
					--need_to_kick;
				}
			}
			
			/* pass 2: nominate bots on TF_TEAM_BLUE to be kicked */
			for (auto bot : mvm_bots) {
				if (need_to_kick <= 0) break;
				
				if (bot->GetTeamNumber() == TF_TEAM_BLUE) {
					bots_to_kick.AddToTail(bot);
					--need_to_kick;
				}
			}
			
			/* pass 3: nominate bots on TF_TEAM_RED to be kicked */
			if (cvar_fix_red.GetBool()) {
				for (auto bot : mvm_bots) {
					if (need_to_kick <= 0) break;
					
					if (bot->GetTeamNumber() == TF_TEAM_RED) {
						bots_to_kick.AddToTail(bot);
						--need_to_kick;
					}
				}
			}
			
			/* now, kick the bots we nominated */
			for (auto bot : bots_to_kick) {
				engine->ServerCommand(CFmtStr("kickid %d\n", bot->GetUserID()));
			}
		}
	}
	
	
	RefCount rc_CTFBotSpawner_Spawn;
	DETOUR_DECL_MEMBER(int, CTFBotSpawner_Spawn, const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents)
	{
		SCOPED_INCREMENT(rc_CTFBotSpawner_Spawn);
		return DETOUR_MEMBER_CALL(CTFBotSpawner_Spawn)(where, ents);
	}
	
	// rewrite this function entirely, with some changes:
	// - rather than including any CTFPlayer that IsBot, only count TFBots
	// - don't exclude bots who are on TF_TEAM_RED if they have TF_COND_REPROGRAMMED
	// ALSO, do a hacky thing at the end so we can redo the MvM bots-over-quota logic ourselves
	DETOUR_DECL_STATIC(int, CPopulationManager_CollectMvMBots, CUtlVector<CTFPlayer *> *mvm_bots)
	{
		mvm_bots->RemoveAll();
		
		for (int i = 1; i <= gpGlobals->maxClients; ++i) {
			CTFBot *bot = ToTFBot(UTIL_PlayerByIndex(i));
			if (bot == nullptr)      continue;
			if (ENTINDEX(bot) == 0)  continue;
			if (!bot->IsBot())       continue;
			if (!bot->IsConnected()) continue;
			
			if (bot->GetTeamNumber() == TF_TEAM_RED) {
				if (cvar_fix_red.GetBool() && bot->m_Shared->InCond(TF_COND_REPROGRAMMED)) {
					/* include */
				} else {
					/* exclude */
					continue;
				}
			}
			
			mvm_bots->AddToTail(bot);
		}
		
		if (rc_CTFBotSpawner_Spawn > 0) {
			/* do the bots-over-quota logic ourselves */
			CheckForMaxInvadersAndKickExtras(*mvm_bots);
			
			/* ensure that the original code won't do its own bots-over-quota logic */
			mvm_bots->RemoveAll();
		}
		DevMsg("Collect MVM resulted in %d bots\n", mvm_bots->Count());
		return mvm_bots->Count();
	}
	
	// rewrite this function entirely, with some changes:
	// - use the overridden max bot count, rather than a hardcoded 22
	DETOUR_DECL_MEMBER(void, CPopulationManager_AllocateBots)
	{
		auto popmgr = reinterpret_cast<CPopulationManager *>(this);
		
		if (popmgr->m_bAllocatedBots) return;
		
		CUtlVector<CTFPlayer *> mvm_bots;
		int num_bots = CPopulationManager::CollectMvMBots(&mvm_bots);
		
		if (num_bots > 0) {
			Warning("%d bots were already allocated some how before CPopulationManager::AllocateBots was called\n", num_bots);
		}
		
		while (num_bots < GetMvMInvaderLimit()) {
			CTFBot *bot = NextBotCreatePlayerBot<CTFBot>("TFBot", false);
			if (bot != nullptr) {
				bot->ChangeTeam(TEAM_SPECTATOR, false, true, false);
			}
			
			++num_bots;
		}
		
		popmgr->m_bAllocatedBots = true;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:Robot_Limit")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBotSpawner_Spawn,               "CTFBotSpawner::Spawn");
			MOD_ADD_DETOUR_STATIC(CPopulationManager_CollectMvMBots, "CPopulationManager::CollectMvMBots");
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_AllocateBots,   "CPopulationManager::AllocateBots");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_robot_limit", "0", FCVAR_NOTIFY,
		"Mod: modify/enhance/fix some population manager code related to the 22-robot limit in MvM",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
