#include "mod.h"
#include "stub/tfbot.h"
#include "stub/populators.h"
#include "stub/gamerules.h"
#include "mod/pop/popmgr_extensions.h"
#include "mod/mvm/player_limit.h"
#include "util/scope.h"
#include "util/iterate.h"
#include "util/admin.h"

namespace Mod::Pop::WaveSpawn_Extensions
{
	bool IsEnabled();
}

namespace Mod::Etc::Extra_Player_Slots
{
	void SetForceCreateAtSlot(int slot);
	bool ExtraSlotsEnabledForBots();
}

namespace Mod::MvM::Robot_Limit
{
	void CheckForMaxInvadersAndKickExtras(CUtlVector<CTFPlayer *>& mvm_bots);
	int CollectMvMBots(CUtlVector<CTFPlayer *> *mvm_bots, bool collect_red);

	ConVar cvar_fix_red("sig_mvm_robot_limit_fix_red", "0", FCVAR_NOTIFY,
		"Mod: fix problems with enforcement of the MvM robot limit when bots are on red team");

	bool allocate_round_start = false;
	bool hibernated = false;
	
	ConVar *tf_mvm_max_invaders;
	int GetMvMInvaderLimit() { return tf_mvm_max_invaders->GetInt(); }

	THINK_FUNC_DECL(SpawnBots)
	{
		allocate_round_start = true;
		if (!hibernated)
			reinterpret_cast<CPopulationManager *>(this)->AllocateBots();
		allocate_round_start = false;
	}

	THINK_FUNC_DECL(UpdateRobotCounts)
	{
		CUtlVector<CTFPlayer *> mvm_bots;
		CollectMvMBots(&mvm_bots, cvar_fix_red.GetBool());
		CheckForMaxInvadersAndKickExtras(mvm_bots);
		THINK_FUNC_SET(g_pPopulationManager, SpawnBots, gpGlobals->curtime + 0.12f);
	}

	THINK_FUNC_DECL(SpawnBotsSecondPass)
	{
		CUtlVector<CTFPlayer *> mvm_bots;
		int num_bots = CPopulationManager::CollectMvMBots(&mvm_bots);

		while (num_bots < GetMvMInvaderLimit()) {
			CTFBot *bot = NextBotCreatePlayerBot<CTFBot>("TFBot", false);
			if (bot != nullptr) {
				bot->ChangeTeam(TEAM_SPECTATOR, false, true, false);
			}
			num_bots++;
		}
	}


	// Get max slot number that the bots may occupy
	int GetMaxAllowedSlot()
	{
		//return gpGlobals->maxClients;
		if (!Mod::Etc::Extra_Player_Slots::ExtraSlotsEnabledForBots()) return MAX_PLAYERS;
		
		static ConVarRef tv_enable("tv_enable");
		int red, blu, spectators, robots;
		return Mod::MvM::Player_Limit::GetSlotCounts(red, blu, spectators, robots);
	}
	
	
	// reimplement the MvM bots-over-quota logic, with some changes:
	// - 
	// - add a third pass to the collect-bots-to-kick logic for bots on TF_TEAM_RED
	void CheckForMaxInvadersAndKickExtras(CUtlVector<CTFPlayer *>& mvm_bots)
	{
		// When extra bot slots are enabled, always kick bots in slots considered reserved for players
		static ConVarRef tv_enable("tv_enable");

		int red, blu, spectators, robots;
		Mod::MvM::Player_Limit::GetSlotCounts(red, blu, spectators, robots);

		if (Mod::Etc::Extra_Player_Slots::ExtraSlotsEnabledForBots()) {

			int playerReservedSlots = red + blu + spectators;

			for (int i = 0; i < mvm_bots.Count(); i++) {
				if (ENTINDEX(mvm_bots[i]) <= playerReservedSlots) {
					engine->ServerCommand(CFmtStr("kickid %d %s  [bot slot: %d. reserved slot count: %d]\n", mvm_bots[i]->GetUserID(), "kick bot in player reserved slot", ENTINDEX(mvm_bots[i]), playerReservedSlots));
					mvm_bots.Remove(i);
					i--;
				}
			}
		}
		
		int maxAllowedSlot = GetMaxAllowedSlot();
		for (int i = 0; i < mvm_bots.Count(); i++) {
			if (ENTINDEX(mvm_bots[i]) > MAX_PLAYERS && ENTINDEX(mvm_bots[i]) > maxAllowedSlot) {
				
				engine->ServerCommand(CFmtStr("kickid %d kick bot over the maximum allowed slot [bot slot: %d, max slot: %d]\n", mvm_bots[i]->GetUserID(), ENTINDEX(mvm_bots[i]), maxAllowedSlot));
				mvm_bots.Remove(i);
				i--;
			}
		}

		if (mvm_bots.Count() < GetMvMInvaderLimit()) return;
		
		if (mvm_bots.Count() > GetMvMInvaderLimit()) {
			CUtlVector<CTFPlayer *> bots_to_kick;
			int need_to_kick = (mvm_bots.Count() - GetMvMInvaderLimit());
			DevMsg("Need to kick %d bots\n", need_to_kick);
			
			/* pass 2: nominate bots on TEAM_SPECTATOR to be kicked */
			for (int i = 0; i < mvm_bots.Count() && need_to_kick > 0; i++) {
				if (mvm_bots[i]->GetTeamNumber() == TEAM_SPECTATOR /*&& ENTINDEX(bot) <= 33*/) {
					bots_to_kick.AddToTail(mvm_bots[i]);
					mvm_bots.Remove(i);
					i--;
					--need_to_kick;
				}
			}
			
			/* pass 3: nominate bots on TF_TEAM_RED to be kicked */
			if (cvar_fix_red.GetBool()) {
				for (int i = 0; i < mvm_bots.Count() && need_to_kick > 0; i++) {
					
					if (mvm_bots[i]->GetTeamNumber() == TF_TEAM_RED /*&& ENTINDEX(bot) <= 33*/) {
						bots_to_kick.AddToTail(mvm_bots[i]);
						mvm_bots.Remove(i);
						i--;
						--need_to_kick;
					}
				}
			}
			/* pass 4: nominate dead bots to be kicked */
			for (int i = 0; i < mvm_bots.Count() && need_to_kick > 0; i++) {
				
				if (!mvm_bots[i]->IsAlive() /*&& ENTINDEX(bot) <= 33*/) {
					bots_to_kick.AddToTail(mvm_bots[i]);
					mvm_bots.Remove(i);
					i--;
					--need_to_kick;
				}
			}
			/* pass 5: nominate bots on TF_TEAM_BLUE to be kicked */
			for (int i = 0; i < mvm_bots.Count() && need_to_kick > 0; i++) {
				
				if (mvm_bots[i]->GetTeamNumber() == TF_TEAM_BLUE /*&& ENTINDEX(bot) <= 33*/) {
					bots_to_kick.AddToTail(mvm_bots[i]);
					mvm_bots.Remove(i);
					i--;
					--need_to_kick;
				}
			}

			/* now, kick the bots we nominated */
			for (auto bot : bots_to_kick) {
				engine->ServerCommand(CFmtStr("kickid %d %s\n", bot->GetUserID(), "kick nominated bot over the limit"));
			}
		}
	}
	
	
	// rewrite this function entirely, with some changes:
	// - rather than including any CTFPlayer that IsBot, only count TFBots
	// - don't exclude bots who are on TF_TEAM_RED if they have TF_COND_REPROGRAMMED
	// ALSO, do a hacky thing at the end so we can redo the MvM bots-over-quota logic ourselves

	// Returns red bots collected
	int CollectMvMBots(CUtlVector<CTFPlayer *> *mvm_bots, bool collect_red) {
		mvm_bots->RemoveAll();
		int count_red = 0;
		int maxAllowedSlot = gpGlobals->maxClients;//GetMaxAllowedSlot();
		for (int i = 1; i <= maxAllowedSlot ; i++) {
			CTFBot *bot = ToTFBot(UTIL_PlayerByIndex(i));
			if (bot == nullptr)      continue;
			if (ENTINDEX(bot) == 0)  continue;
			if (!bot->IsBot())       continue;
			if (!bot->IsConnected()) continue;

			if (bot->GetTeamNumber() == TF_TEAM_RED) {
				if (collect_red /*&& bot->m_Shared->InCond(TF_COND_REPROGRAMMED)*/) {
					count_red += 1;
				} else {
					/* exclude */
					continue;
				}
			}

			mvm_bots->AddToTail(bot);
		}
		return count_red;
	}

	RefCount rc_CPopulationManager_CollectMvMBots;
	DETOUR_DECL_STATIC(int, CPopulationManager_CollectMvMBots, CUtlVector<CTFPlayer *> *mvm_bots)
	{
		CollectMvMBots(mvm_bots, cvar_fix_red.GetBool());
		
		if (rc_CPopulationManager_CollectMvMBots == 0) {
			/* do the bots-over-quota logic ourselves */
			CheckForMaxInvadersAndKickExtras(*mvm_bots);
			
			/* ensure that the original code won't do its own bots-over-quota logic */
			int num_bots = mvm_bots->Count();
		}
		DevMsg("Collect MVM resulted in %d bots\n", mvm_bots->Count());
		return mvm_bots->Count();
	}
	
	// rewrite this function entirely, with some changes:
	// - use the overridden max bot count, rather than a hardcoded 22

	DETOUR_DECL_MEMBER(void, CPopulationManager_AllocateBots)
	{
		
		auto popmgr = reinterpret_cast<CPopulationManager *>(this);

		SCOPED_INCREMENT(rc_CPopulationManager_CollectMvMBots);
		
		if (!allocate_round_start) return;
		
		CUtlVector<CTFPlayer *> mvm_bots;
		int num_bots = CPopulationManager::CollectMvMBots(&mvm_bots);
		
		if (num_bots > 0 && !allocate_round_start) {
			Warning("%d bots were already allocated some how before CPopulationManager::AllocateBots was called\n", num_bots);
		}
		
		
		CheckForMaxInvadersAndKickExtras(mvm_bots);
		num_bots = mvm_bots.Count();
		bool full_success = true;

		while (num_bots < GetMvMInvaderLimit()) {
			CTFBot *bot = NextBotCreatePlayerBot<CTFBot>("TFBot", false);
			if (bot != nullptr) {
				bot->ChangeTeam(TEAM_SPECTATOR, false, true, false);
			}
			// Kick spectator players if the player limit is exceeded, sorry
			else {
				full_success = false;
				bool kicked = false;
				ForEachTFPlayer([&](CTFPlayer *player) {
					if (player->GetTeamNumber() < 2 && player->IsRealPlayer() && !PlayerIsSMAdmin(player)) {
						engine->ServerCommand(CFmtStr("kickid %d %s\n", player->GetUserID(), "Exceeded total player limit for the mission"));
						kicked = true;
						return false;
					}
					return true;
				});
			}
			
			++num_bots;
		}
		// Failed to spawn all bots, try again later
		if (!full_success) {
			THINK_FUNC_SET(g_pPopulationManager, SpawnBotsSecondPass, gpGlobals->curtime + 0.05f);
		}
		
		popmgr->m_bAllocatedBots = true;
	}
	//Restore the original max bot counts
	RefCount rc_JumpToWave;

	DETOUR_DECL_MEMBER(void, CPopulationManager_JumpToWave, unsigned int wave, float f1)
	{
		SCOPED_INCREMENT(rc_JumpToWave);
		//DevMsg("[%8.3f] JumpToWaveRobotlimit\n", gpGlobals->curtime);
		DETOUR_MEMBER_CALL(wave, f1);
		

		THINK_FUNC_SET(g_pPopulationManager, SpawnBots, gpGlobals->curtime + 0.12f);

	}
	
	DETOUR_DECL_MEMBER(void, CPopulationManager_WaveEnd, bool b1)
	{
		//DevMsg("[%8.3f] WaveEnd\n", gpGlobals->curtime);
		DETOUR_MEMBER_CALL(b1);

		CUtlVector<CTFPlayer *> mvm_bots;
		CollectMvMBots(&mvm_bots, cvar_fix_red.GetBool());
		CheckForMaxInvadersAndKickExtras(mvm_bots);
	}
	
	DETOUR_DECL_MEMBER(void, CMannVsMachineStats_RoundEvent_WaveEnd, bool success)
	{
		DETOUR_MEMBER_CALL(success);
		if (!success && rc_JumpToWave == 0) {
			//DevMsg("[%8.3f] RoundEvent_WaveEnd\n", gpGlobals->curtime);
			CUtlVector<CTFPlayer *> mvm_bots;
			CollectMvMBots(&mvm_bots, cvar_fix_red.GetBool());
			CheckForMaxInvadersAndKickExtras(mvm_bots);
		}
	}

	DETOUR_DECL_MEMBER(void, CPopulationManager_StartCurrentWave)
	{
		DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(void, CGameServer_SetHibernating, bool hibernate)
	{
		if (!hibernate && hibernated)
		{
			if (g_pPopulationManager != nullptr)
				THINK_FUNC_SET(g_pPopulationManager, SpawnBots, gpGlobals->curtime + 0.12f);
		}
		hibernated = hibernate;
		DETOUR_MEMBER_CALL(hibernate);
	}

	static void MaxInvadersChangeCallback( IConVar *pConVar, const char *pOldValue, float flOldValue )
	{
		if (pConVar == tf_mvm_max_invaders) {
			if (g_pPopulationManager != nullptr)
				THINK_FUNC_SET(g_pPopulationManager, SpawnBots, gpGlobals->curtime + 0.12f);
		}
	}

	class CMod : public IMod, public IModCallbackListener
	{
	public:
		CMod() : IMod("MvM:Robot_Limit")
		{
			//MOD_ADD_DETOUR_MEMBER(CTFBotSpawner_Spawn,               "CTFBotSpawner::Spawn");
			MOD_ADD_DETOUR_STATIC(CPopulationManager_CollectMvMBots, "CPopulationManager::CollectMvMBots");
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_AllocateBots,   "CPopulationManager::AllocateBots");

			MOD_ADD_DETOUR_MEMBER(CPopulationManager_JumpToWave,          "CPopulationManager::JumpToWave");
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_WaveEnd,             "CPopulationManager::WaveEnd");
			MOD_ADD_DETOUR_MEMBER(CMannVsMachineStats_RoundEvent_WaveEnd, "CMannVsMachineStats::RoundEvent_WaveEnd");

			MOD_ADD_DETOUR_MEMBER(CGameServer_SetHibernating, "CGameServer::SetHibernating");
		}

		virtual void OnEnable() override
		{
			icvar->InstallGlobalChangeCallback(MaxInvadersChangeCallback);
		}

		virtual void OnDisable() override
		{
			icvar->RemoveGlobalChangeCallback(MaxInvadersChangeCallback);
		}

		virtual bool OnLoad() override
		{
			tf_mvm_max_invaders = icvar->FindVar("tf_mvm_max_invaders");
			return true;
		}

		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

		virtual void LevelInitPreEntity() override
		{
		}
		
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_robot_limit", "0", FCVAR_NOTIFY,
		"Mod: modify/enhance/fix some population manager code related to the robot limit in MvM",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
