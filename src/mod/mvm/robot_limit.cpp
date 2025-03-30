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
		DevMsg("Changed\n");
		THINK_FUNC_SET(g_pPopulationManager, SpawnBots, gpGlobals->curtime + 0.12f);
	}

	ConVar cvar_override("sig_mvm_robot_limit_override", "22", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Mod: override the max number of MvM robots that are allowed to be spawned at once (normally 22)",
		true, 0, false, 0,
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			/* ensure that slots are cleared up when this convar is decreased */
			if (g_pPopulationManager != nullptr) {
				THINK_FUNC_SET(g_pPopulationManager, UpdateRobotCounts, gpGlobals->curtime + 0.01);
			}
			
		});

	int GetMvMInvaderLimit() { return cvar_override.GetInt(); }

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
	// - use the overridden max bot count, rather than a hardcoded 22
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
					engine->ServerCommand(CFmtStr("kickid %d kick bot in player reserved slot\n", mvm_bots[i]->GetUserID()));
					mvm_bots.Remove(i);
					i--;
				}
			}
		}

		// Kick bots over the slot 33

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
			for (auto bot : mvm_bots) {
				if (need_to_kick <= 0) break;
				
				if (bot->GetTeamNumber() == TEAM_SPECTATOR /*&& ENTINDEX(bot) <= 33*/) {
					bots_to_kick.AddToTail(bot);
					--need_to_kick;
				}
			}
			
			/* pass 3: nominate bots on TF_TEAM_RED to be kicked */
			if (cvar_fix_red.GetBool()) {
				for (auto bot : mvm_bots) {
					if (need_to_kick <= 0) break;
					
					if (bot->GetTeamNumber() == TF_TEAM_RED /*&& ENTINDEX(bot) <= 33*/) {
						bots_to_kick.AddToTail(bot);
						--need_to_kick;
					}
				}
			}
			/* pass 4: nominate dead bots to be kicked */
			for (auto bot : mvm_bots) {
				if (need_to_kick <= 0) break;
				
				if (!bot->IsAlive() /*&& ENTINDEX(bot) <= 33*/) {
					bots_to_kick.AddToTail(bot);
					--need_to_kick;
				}
			}
			/* pass 5: nominate bots on TF_TEAM_BLUE to be kicked */
			for (auto bot : mvm_bots) {
				if (need_to_kick <= 0) break;
				
				if (bot->GetTeamNumber() == TF_TEAM_BLUE /*&& ENTINDEX(bot) <= 33*/) {
					bots_to_kick.AddToTail(bot);
					--need_to_kick;
				}
			}

			/* now, kick the bots we nominated */
			for (auto bot : bots_to_kick) {
				engine->ServerCommand(CFmtStr("kickid %d kick nominated bot over the limit\n", bot->GetUserID()));
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
			
			/* bot spawner has a hardcoded 22 bots check */
			if (num_bots >= 22 || num_bots >= GetMvMInvaderLimit()) {
				mvm_bots->SetCountNonDestructively(22);
			}
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
		
		while (num_bots < GetMvMInvaderLimit()) {
			// Kick spectator players if the player limit is exceeded, sorry
			if (sv->GetNumClients() >= sv->GetMaxClients()) {
				bool kicked = false;
				ForEachTFPlayer([&](CTFPlayer *player) {
					if (player->GetTeamNumber() < 2 && player->IsRealPlayer() && !PlayerIsSMAdmin(player)) {
						engine->ServerCommand(CFmtStr("kickid %d %s\n", player->GetUserID(), "Exceeded total player limit for the mission"));
						engine->ServerExecute();
						return false;
					}
					return true;
				});
			}

			CTFBot *bot = NextBotCreatePlayerBot<CTFBot>("TFBot", false);
			if (bot != nullptr) {
				bot->ChangeTeam(TEAM_SPECTATOR, false, true, false);
			}
			
			++num_bots;
		}

		// To prevent client crashes when 33+ slots are used, create all bots at extra slots at once. They will still be ignored if robot limit is not high enough
		// if (has_extra_bot_mission) {
		// 	for (int i = 33; i < gpGlobals->maxClients; i++) {
		// 		if (UTIL_PlayerByIndex(i + 1) == nullptr) {
		// 			Mod::Etc::Extra_Player_Slots::SetForceCreateAtSlot(i);
		// 			CTFBot *bot = NextBotCreatePlayerBot<CTFBot>("TFBot", false);
		// 			Mod::Etc::Extra_Player_Slots::SetForceCreateAtSlot(-1);
		// 			if (bot != nullptr) {
		// 				bot->ChangeTeam(TEAM_SPECTATOR, false, true, false);
		// 			}
		// 		}
		// 	}
		// }
		
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

	DETOUR_DECL_MEMBER(ActionResult< CTFBot >, CTFBotDead_Update, CTFBot *bot, float interval)
	{
		auto result = DETOUR_MEMBER_CALL(bot,interval);
		if (result.transition == ActionTransition::DONE) {
			CUtlVector<CTFPlayer *> mvm_bots;
			CollectMvMBots(&mvm_bots, cvar_fix_red.GetBool());
			if (mvm_bots.Count() > GetMvMInvaderLimit()) {
				engine->ServerCommand(CFmtStr("kickid %d\n", bot->GetUserID()));
			}
		}
		return result;
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

	CWaveSpawnPopulator *populator_parse = nullptr;
	DETOUR_DECL_MEMBER(bool, CWaveSpawnPopulator_Parse, KeyValues *kv_orig)
	{
		populator_parse = reinterpret_cast<CWaveSpawnPopulator *>(this);
		bool result = DETOUR_MEMBER_CALL(kv_orig);
		populator_parse = nullptr;
		
		return result;
	}

	DETOUR_DECL_MEMBER(bool, CTFBotSpawner_Parse, KeyValues *kv_orig)
	{
		if (populator_parse != nullptr && Mod::Pop::WaveSpawn_Extensions::IsEnabled()) {
			// Unused variable, now used to tell if the wavespawn contains a tfbot spawner
			populator_parse->m_bHasTFBotSpawner = true;
		}
		return DETOUR_MEMBER_CALL(kv_orig);
	}

	DETOUR_DECL_MEMBER(void, CWaveSpawnPopulator_Update)
	{
		auto wavespawn = reinterpret_cast<CWaveSpawnPopulator *>(this);
		int offset = 0;
		int &slots = CWaveSpawnPopulator::m_reservedPlayerSlotCount.GetRef();
		if (wavespawn->m_state == CWaveSpawnPopulator::SPAWNING) {
			offset = - (GetMvMInvaderLimit() - 22);
		}
		slots += offset;
		DETOUR_MEMBER_CALL();
		slots -= offset;
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

			MOD_ADD_DETOUR_MEMBER(CWaveSpawnPopulator_Parse,  "CWaveSpawnPopulator::Parse");
			MOD_ADD_DETOUR_MEMBER(CTFBotSpawner_Parse,        "CTFBotSpawner::Parse");
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CWaveSpawnPopulator_Update, "CWaveSpawnPopulator::Update", LOW);

			//MOD_ADD_DETOUR_MEMBER(CTFBotDead_Update, "CTFBotDead::Update");

			MOD_ADD_DETOUR_MEMBER(CGameServer_SetHibernating, "CGameServer::SetHibernating");
		}

		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

		virtual void LevelInitPreEntity() override
		{
		}
		
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_robot_limit", "0", FCVAR_NOTIFY,
		"Mod: modify/enhance/fix some population manager code related to the 22-robot limit in MvM",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
