#include "mod.h"
#include "util/scope.h"
#include "stub/tfplayer.h"
#include "stub/gamerules.h"
#include "stub/usermessages_sv.h"
#include "stub/particles.h"
#include "stub/extraentitydata.h"
#include "stub/populators.h"
#include "stub/team.h"
#include "util/iterate.h"
#include "util/value_override.h"
#include "util/admin.h"
#include "mod/mvm/player_limit.h"

namespace Mod::Etc::Extra_Player_Slots
{
	bool ExtraSlotsEnabledExternal();
}
namespace Mod::MvM::JoinTeam_Blue_Allow
{
    bool PlayersCanJoinBlueTeam();
}
namespace Mod::MvM::Player_Limit
{

    void TogglePatches();
    void RecalculateSlots();
    void ResetMaxRedTeamPlayers(int resetTo);

    extern ConVar cvar_enable;
    
    void ToggleModActive();

    CValueOverride_ConVar<bool> allowspectators("mp_allowspectators");

    ConVar sig_mvm_spectator_max_players("sig_mvm_spectator_max_players", "-1", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Set max spectator team count. -1 = Default",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){ 
            RecalculateSlots();
            if (((ConVar *)pConVar)->GetInt() < 0 && flOldValue >= 0) {
                allowspectators.Reset();
            }
		});	

    
	int GetMaxSpectators()
	{
		return sig_mvm_spectator_max_players.GetInt();
	}

	int GetMaxNonSpectatorPlayers() {
		int red, blu, spectators, robots;
		GetSlotCounts(red, blu, spectators, robots);
		return red + blu;
	}

    void ResetMaxTotalPlayers(int resetTo);

	void ResetMaxTotalPlayers() {
		if (!cvar_enable.GetBool()) {
			ResetMaxTotalPlayers(0);
			return;
		}
		int red, blu, spectators, robots;
		GetSlotCounts(red, blu, spectators, robots);
		ResetMaxTotalPlayers(red + blu + spectators);
	}

    int maxTotalPlayers = 10;
	THINK_FUNC_DECL(KickPlayersOverLimit)
	{
		if (maxTotalPlayers == 0) return;
		int totalplayers = 0;
		ForEachTFPlayer([&](CTFPlayer *player) {
			if (player->IsRealPlayer())
				totalplayers += 1;
		});
		
		if (totalplayers > maxTotalPlayers) {
			ForEachTFPlayer([&](CTFPlayer *player) {
				if (player->GetTeamNumber() < 2 && totalplayers > maxTotalPlayers && player->IsRealPlayer() && !PlayerIsSMAdmin(player)) {
					engine->ServerCommand(CFmtStr("kickid %d %s\n", player->GetUserID(), "Exceeded total player limit for the mission"));
					totalplayers -= 1;
				}
			});
			ForEachTFPlayer([&](CTFPlayer *player) {
				if (player->GetTeamNumber() >= 2 && totalplayers > maxTotalPlayers && player->IsRealPlayer() && !PlayerIsSMAdmin(player)) {
					engine->ServerCommand(CFmtStr("kickid %d %s\n", player->GetUserID(), "Exceeded total player limit for the mission"));
					totalplayers -= 1;
				}
			});
		}
	}

    CValueOverride_ConVar<int> override_tf_mvm_max_connected_players("tf_mvm_max_connected_players");
    CValueOverride_ConVar<int> override_tf_mvm_defenders_team_size("tf_mvm_defenders_team_size");
	
	void ResetMaxTotalPlayers(int resetTo) {
		maxTotalPlayers = resetTo;
		if (g_pPopulationManager != nullptr) {
			THINK_FUNC_SET(g_pPopulationManager, KickPlayersOverLimit, gpGlobals->curtime);
		}

		if (resetTo > 0)
			override_tf_mvm_max_connected_players.Set(std::max(6, resetTo));
		else 
			override_tf_mvm_max_connected_players.Reset();
	}

    void RecalculateSlots() {
        ToggleModActive();
		ResetMaxTotalPlayers();
    }

	int GetSlotCounts(int &red, int &blu, int &spectators, int &robots) 
	{
		//static ConVarRef tf_mvm_max_connected_players("tf_mvm_max_connected_players");
		static ConVarRef tf_mvm_defenders_team_size("tf_mvm_defenders_team_size");
		static ConVarRef tf_mvm_max_invaders("tf_mvm_max_invaders");

		red = tf_mvm_defenders_team_size.GetInt();
		blu = 0;
		robots = tf_mvm_max_invaders.GetInt();

		spectators = GetMaxSpectators();
		
		static ConVarRef tv_enable("tv_enable");
		int tvSlots = (tv_enable.GetBool() ? 1 : 0);

		static ConVarRef sig_mvm_jointeam_blue_allow("sig_mvm_jointeam_blue_allow");
		static ConVarRef sig_mvm_jointeam_blue_allow_max("sig_mvm_jointeam_blue_allow_max");
		static ConVarRef sig_etc_extra_player_slots_allow_players("sig_etc_extra_player_slots_allow_players");
		static ConVarRef sig_etc_extra_player_slots_allow_bots("sig_etc_extra_player_slots_allow_bots");

		if (sig_mvm_jointeam_blue_allow.GetInt() != 0) {
			if (sig_mvm_jointeam_blue_allow_max.GetInt() > 0) {
				blu = sig_mvm_jointeam_blue_allow_max.GetInt();
			}
			else {
				if (Mod::Etc::Extra_Player_Slots::ExtraSlotsEnabledExternal() && sig_etc_extra_player_slots_allow_players.GetBool()) {
					blu = gpGlobals->maxClients - robots - red - Max(spectators, 0) - tvSlots;
				}
				else if (Mod::Etc::Extra_Player_Slots::ExtraSlotsEnabledExternal() && sig_etc_extra_player_slots_allow_bots.GetBool()) {
					blu = Min(gpGlobals->maxClients - robots - red - Max(spectators, 0) - tvSlots, MAX_PLAYERS - red - Max(spectators, 0) - tvSlots);
				}
				else {
					blu = MAX_PLAYERS - robots - red - Max(spectators, 0) - tvSlots;
				}
				
			}
        } 
		
		static ConVarRef sig_mvm_jointeam_blue_allow_force("sig_mvm_jointeam_blue_allow_force");
		if (sig_mvm_jointeam_blue_allow_force.GetInt() != 0 && sig_mvm_jointeam_blue_allow_max.GetInt() > 0) {
			blu = sig_mvm_jointeam_blue_allow_max.GetInt() >= 0 ? sig_mvm_jointeam_blue_allow_max.GetInt() : gpGlobals->maxClients - robots - Max(spectators, 0);
			red = 0;
		}


		int freeSlots = MAX_PLAYERS - robots - tvSlots;
		if (spectators == -1) {
			spectators = Max(0, freeSlots - red - blu);
		}
		return red + blu + spectators + robots + tvSlots;
	}

	DETOUR_DECL_MEMBER(bool, CTFGameRules_ClientConnected, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
	{
		auto gamerules = reinterpret_cast<CTFGameRules *>(this);
        if (gamerules->IsMannVsMachineMode()) {
            // Fix Source TV client counting as real player
            ForEachTFPlayer([](CTFPlayer *playerL){
                if (playerL->IsHLTV() || playerL->IsReplay()) {
                    playerL->m_fFlags |= FL_FAKECLIENT;
                }
            });
            if (sig_mvm_spectator_max_players.GetInt() >= 0) {
                int spectators = 0;
                int assigned = 0;
                ForEachTFPlayer([&](CTFPlayer *player){
                    if(player->IsRealPlayer()) {
                        if (player->GetTeamNumber() == TF_TEAM_BLUE || player->GetTeamNumber() == TF_TEAM_RED)
                            assigned++;
                        else
                            spectators++;
                    }
                });
                if (spectators >= sig_mvm_spectator_max_players.GetInt() && assigned >= GetMaxNonSpectatorPlayers()) {
                    strcpy(reject, "Exceeded total spectator count of the mission");
                    return false;
                }
            }
        }
		
		bool ret = DETOUR_MEMBER_CALL(pEntity, pszName, pszAddress, reject, maxrejectlen);
        
        if (gamerules->IsMannVsMachineMode()) {
            // Fix Source TV client counting as real player
            ForEachTFPlayer([](CTFPlayer *playerL){
                if (playerL->IsHLTV() || playerL->IsReplay()) {
                    playerL->m_fFlags &= ~(FL_FAKECLIENT);
                }
            });
        }
        return ret;
	}

	DETOUR_DECL_MEMBER(int, CTFGameRules_GetTeamAssignmentOverride, CTFPlayer *pPlayer, int iWantedTeam, bool b1)
	{
		static ConVarRef tf_mvm_defenders_team_size("tf_mvm_defenders_team_size");
		if (TFGameRules()->IsMannVsMachineMode() && pPlayer->IsRealPlayer()
			&& iWantedTeam == TF_TEAM_RED) {
				
			int totalPlayers = 0;
			ForEachTFPlayerOnTeam(TFTeamMgr()->GetTeam(TF_TEAM_RED), [&totalPlayers, pPlayer](CTFPlayer *player){
				if (player->IsRealPlayer() && pPlayer != player) {
					totalPlayers += 1;
				}
			});

			if (totalPlayers < tf_mvm_defenders_team_size.GetInt()) {
				Log( "MVM assigned %s to defending team (%d more slots remaining after us)\n", pPlayer->GetPlayerName(), tf_mvm_defenders_team_size.GetInt() - totalPlayers - 1 );
				// Set Their Currency
				int nRoundCurrency = MannVsMachineStats_GetAcquiredCredits();
				nRoundCurrency += g_pPopulationManager->m_nStartingCurrency;

				// deduct any cash that has already been spent
				int spentCurrency = g_pPopulationManager->GetPlayerCurrencySpent(pPlayer);
				pPlayer->SetCurrency(nRoundCurrency - spentCurrency);
				return TF_TEAM_RED;
			}
			else {
				// no room
				Log( "MVM assigned %s to spectator, all slots for defending team are in use, or reserved for lobby members\n",
				     pPlayer->GetPlayerName() );
				return TEAM_SPECTATOR;
			}
			
		}

		/* it's important to let the call happen, because pPlayer->m_nCurrency
		 * is set to its proper value in the call (stupid, but whatever) */
		auto iResult = DETOUR_MEMBER_CALL(pPlayer, iWantedTeam, b1);
		
		return iResult;
	}

	DETOUR_DECL_MEMBER(void, CTFGCServerSystem_PreClientUpdate)
	{
		int red, blu, spectators, robots;
		GetSlotCounts(red, blu, spectators, robots);

		static CValueOverride_ConVar<int> tf_mvm_defenders_team_size("tf_mvm_defenders_team_size");
		tf_mvm_defenders_team_size.Set(red + blu);
		DETOUR_MEMBER_CALL();
		tf_mvm_defenders_team_size.Reset();
	}

    //CPatch_GetTeamAssignmentOverride patchTeamAssignmentOverride;
    //CPatch_CTFGameRules_ClientConnected patchCTFGameRules_ClientConnected;
    //CPatch_CTFGCServerSystem_PreClientUpdate patchCTFGCServerSystem_PreClientUpdate;

    class CMod : public IMod, IModCallbackListener, IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("MvM:Player_Limit")
		{
            MOD_ADD_DETOUR_MEMBER_PRIORITY(CTFGameRules_GetTeamAssignmentOverride, "CTFGameRules::GetTeamAssignmentOverride", LOWEST);
            MOD_ADD_DETOUR_MEMBER(CTFGameRules_ClientConnected, "CTFGameRules::ClientConnected");
			MOD_ADD_DETOUR_MEMBER(CTFGCServerSystem_PreClientUpdate,   "CTFGCServerSystem::PreClientUpdate");
		}

		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

		virtual void LevelInitPostEntity() override
		{
            if (!TFGameRules()->IsMannVsMachineMode()) {
                allowspectators.Reset();
            }
        }

		virtual void OnDisable() override
		{
			override_tf_mvm_defenders_team_size.Reset();
			override_tf_mvm_max_connected_players.Reset();
			allowspectators.Reset();
		}

        virtual void FrameUpdatePostEntityThink() override
		{
            if (!TFGameRules()->IsMannVsMachineMode()) return;

            if (sig_mvm_spectator_max_players.GetInt() >= 0) {
				int spectators = 0;
				ForEachTFPlayer([&](CTFPlayer *player){
					
					if(player->IsRealPlayer()) {
						if (!(player->GetTeamNumber() == TF_TEAM_BLUE || player->GetTeamNumber() == TF_TEAM_RED))
							spectators++;
					}
				});
				if (spectators > sig_mvm_spectator_max_players.GetInt()) {
					ForEachTFPlayer([&](CTFPlayer *player) {
						if (player->GetTeamNumber() < TF_TEAM_RED && spectators > sig_mvm_spectator_max_players.GetInt() && player->IsRealPlayer()) {
							player->HandleCommand_JoinTeam("red");

							if (player->GetTeamNumber() >= TF_TEAM_RED) {
								spectators -= 1;
							}
							else if (!PlayerIsSMAdmin(player)){
								// Kick if cannot switch to red team
								engine->ServerCommand(CFmtStr("kickid %d %s (%d max spec, %d cur spec)\n", player->GetUserID(), "Exceeded total player limit for the mission ",sig_mvm_spectator_max_players.GetInt(), spectators));
								spectators -= 1;
							}
						}
					});
				}
				allowspectators.Set(allowspectators.GetOriginalValue() && spectators < sig_mvm_spectator_max_players.GetInt());
			}
        }
	};
	CMod s_Mod;

	void ToggleModActive() {
        bool activate = cvar_enable.GetInt() > 0 || sig_mvm_spectator_max_players.GetInt() >= 0;
        if (Mod::MvM::JoinTeam_Blue_Allow::PlayersCanJoinBlueTeam()) {
            activate = true;
        }
		s_Mod.Toggle(activate);
    }

	ConVar cvar_enable("sig_mvm_player_limit_change", "0", FCVAR_NONE,
		"Mod: Change mvm player limits. Set to -1 to never allow missions to change this value",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			RecalculateSlots();
		});
}