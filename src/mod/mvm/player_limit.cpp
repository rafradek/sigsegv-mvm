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


namespace Mod::MvM::JoinTeam_Blue_Allow
{
    bool PlayersCanJoinBlueTeam();
}
namespace Mod::MvM::Player_Limit
{
    // Overrides maximum number of red players that can join at the moment
	int iGetTeamAssignmentOverride = 6;
	#if defined _LINUX

	static constexpr uint8_t s_Buf_GetTeamAssignmentOverride[] = {
		0x8D, 0xB6, 0x00, 0x00, 0x00, 0x00, // +0000
		0xB8, 0x06, 0x00, 0x00, 0x00, // +0006
		0x2B, 0x45, 0xC4, // +000B
		0x29, 0xF0, // +000E
		0x85, 0xC0, // +0010

		//0xc7, 0x84, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // +0000  mov dword ptr [ebx+eax*4+m_Visuals],0x00000000
		//0x8b, 0x04, 0x85, 0x00, 0x00, 0x00, 0x00,                         // +000B  mov eax,g_TeamVisualSections[eax*4]
	};

	struct CPatch_GetTeamAssignmentOverride : public CPatch
	{
		CPatch_GetTeamAssignmentOverride() : CPatch(sizeof(s_Buf_GetTeamAssignmentOverride)) {}
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_GetTeamAssignmentOverride);
			
			mask.SetRange(0x06 + 1, 4, 0x00);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf .SetRange(0x06+1, 1, iGetTeamAssignmentOverride);
			mask.SetRange(0x06+1, 4, 0xff);
			
			return true;
		}
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			buf .SetRange(0x06+1, 1, iGetTeamAssignmentOverride);
			
			return true;
		}
		virtual const char *GetFuncName() const override   { return "CTFGameRules::GetTeamAssignmentOverride"; }
		virtual uint32_t GetFuncOffMin() const override    { return 0x0100; }
		virtual uint32_t GetFuncOffMax() const override    { return 0x0300; }
	};

	#elif defined _WINDOWS

	using CExtract_GetTeamAssignmentOverride = IExtractStub;

	#endif

    // Overrides maximum number of players that may join the server
	int iClientConnected = 9;
	#if defined _LINUX

	static constexpr uint8_t s_Buf_CTFGameRules_ClientConnected[] = {
		0x31, 0xC0, // +0000
		0x83, 0xFE, 0x09, // +0002
	};

	struct CPatch_CTFGameRules_ClientConnected : public CPatch
	{
		CPatch_CTFGameRules_ClientConnected() : CPatch(sizeof(s_Buf_CTFGameRules_ClientConnected)) {}
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CTFGameRules_ClientConnected);
			
			mask.SetRange(0x02 + 2, 1, 0x00);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf .SetRange(0x02+2, 1, iClientConnected);
			mask.SetRange(0x02+2, 1, 0xff);
			
			return true;
		}
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			buf .SetRange(0x02+2, 1, iClientConnected);
			
			return true;
		}
		virtual const char *GetFuncName() const override   { return "CTFGameRules::ClientConnected"; }
		virtual uint32_t GetFuncOffMin() const override    { return 0x0030; }
		virtual uint32_t GetFuncOffMax() const override    { return 0x0100; }
	};

	#elif defined _WINDOWS

	using CExtract_GetTeamAssignmentOverride = IExtractStub;

	#endif

    // Overrides max visible player count
	int iPreClientUpdate = 6;
	#if defined _LINUX

	static constexpr uint8_t s_Buf_CTFGCServerSystem_PreClientUpdate[] = {
		0x8B, 0x5D, 0xB0, // +0000
		0x83, 0xC3, 0x06, // +0003
	};

	struct CPatch_CTFGCServerSystem_PreClientUpdate : public CPatch
	{
		CPatch_CTFGCServerSystem_PreClientUpdate() : CPatch(sizeof(s_Buf_CTFGCServerSystem_PreClientUpdate)) {}
		
		virtual bool GetVerifyInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf.CopyFrom(s_Buf_CTFGCServerSystem_PreClientUpdate);
			
			mask.SetRange(0x03 + 2, 1, 0x00);
			
			return true;
		}
		
		virtual bool GetPatchInfo(ByteBuf& buf, ByteBuf& mask) const override
		{
			buf .SetRange(0x03+2, 1, iPreClientUpdate);
			mask.SetRange(0x03+2, 1, 0xff);
			
			return true;
		}
		virtual bool AdjustPatchInfo(ByteBuf& buf) const override
		{
			buf .SetRange(0x03+2, 1, iPreClientUpdate);
			
			return true;
		}
		virtual const char *GetFuncName() const override   { return "CTFGCServerSystem::PreClientUpdate"; }
		virtual uint32_t GetFuncOffMin() const override    { return 0x0350; }
		virtual uint32_t GetFuncOffMax() const override    { return 0x0600; }
	};

	#elif defined _WINDOWS

	using CPatch_CTFGCServerSystem_PreClientUpdate = IExtractStub;

	#endif

    void TogglePatches();
    void RecalculateSlots();
    void ResetMaxRedTeamPlayers(int resetTo);

    extern ConVar cvar_enable;
    extern ConVar cvar_max_red_players;
    
    void ToggleModActive() {
        if (cvar_enable.GetInt() != 0 && cvar_enable.GetInt() != 2) {
            return;
        }
        bool activate = false;
        if (cvar_max_red_players.GetInt() > 0) {
            activate = true;
        }
        if (Mod::MvM::JoinTeam_Blue_Allow::PlayersCanJoinBlueTeam()) {
            activate = true;
        }
        cvar_enable.SetValue(activate ? 2 : 0);
    }

	ConVar cvar_max_red_players("sig_mvm_red_team_max_players", "0", FCVAR_NOTIFY,
		"Set max red team count. 0 = Default",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){ 

			if (cvar_max_red_players.GetInt() > 0) {
				ResetMaxRedTeamPlayers(cvar_max_red_players.GetInt());
			}
			else {
				ResetMaxRedTeamPlayers(6);
			}

			RecalculateSlots();
		});	


    CValueOverride_ConVar<bool> allowspectators("mp_allowspectators");

    ConVar sig_mvm_spectator_max_players("sig_mvm_spectator_max_players", "-1", FCVAR_NOTIFY,
		"Set max spectator team count. -1 = Default",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){ 
            RecalculateSlots();

            if (((ConVar *)pConVar)->GetInt() < 0 && flOldValue >= 0) {
                allowspectators.Reset();
            }
		});	
	
    
	THINK_FUNC_DECL(MoveRedPlayers)
	{
		int redplayers = 0;
		ForEachTFPlayer([&](CTFPlayer *player) {
			if (player->GetTeamNumber() == TF_TEAM_RED && player->IsRealPlayer())
				redplayers += 1;
		});
		if (redplayers > iGetTeamAssignmentOverride) {
			ForEachTFPlayer([&](CTFPlayer *player) {
				if (player->GetTeamNumber() == TF_TEAM_RED && redplayers > iGetTeamAssignmentOverride && player->IsRealPlayer() && !PlayerIsSMAdmin(player)) {
					player->ForceChangeTeam(TEAM_SPECTATOR, false);
					redplayers -= 1;
				}
			});
		}
	}

	void ResetMaxRedTeamPlayers(int resetTo) {
        
		if (g_pPopulationManager != nullptr) {
			THINK_FUNC_SET(g_pPopulationManager, MoveRedPlayers, gpGlobals->curtime);
		}
		if (resetTo > 0) {
			iGetTeamAssignmentOverride = resetTo;
		}
		else {
			iGetTeamAssignmentOverride = 6;
		}
		TogglePatches();
	}
    
	int GetMaxSpectators()
	{
		return sig_mvm_spectator_max_players.GetBool();
	}

	int GetMaxNonSpectatorPlayers() {
		int red, blu, spectators, robots;
		GetSlotCounts(red, blu, spectators, robots);
        Msg("red %d blu %d\n", red, blu);
		return red + blu;
	}

    void ResetMaxTotalPlayers(int resetTo);

	void ResetMaxTotalPlayers() {
		int red, blu, spectators, robots;
		GetSlotCounts(red, blu, spectators, robots);
		ResetMaxTotalPlayers(red + blu + spectators);
	}

    int maxTotalPlayers = 10;
	THINK_FUNC_DECL(KickPlayersOverLimit)
	{
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
	
	void ResetMaxTotalPlayers(int resetTo) {
		maxTotalPlayers = resetTo;
		if (g_pPopulationManager != nullptr) {
			THINK_FUNC_SET(g_pPopulationManager, KickPlayersOverLimit, gpGlobals->curtime);
		}
		if (resetTo > 0)
			iClientConnected = std::max(5, resetTo - 1);
		else 
			iClientConnected = 9;
		TogglePatches();
        SetVisibleMaxPlayers();
	}

	void SetVisibleMaxPlayers() {

		iPreClientUpdate = GetMaxNonSpectatorPlayers();
		TogglePatches();
	}

    void RecalculateSlots() {
        ToggleModActive();
        SetVisibleMaxPlayers();
		ResetMaxTotalPlayers();
    }

	int GetSlotCounts(int &red, int &blu, int &spectators, int &robots) 
	{
		red = 6;
		spectators = 4;
		blu = 0;
		if (cvar_max_red_players.GetInt() > 0)
			red = cvar_max_red_players.GetInt();

		robots = 22;
		static ConVarRef sig_mvm_robot_limit("sig_mvm_robot_limit");
		static ConVarRef sig_mvm_robot_limit_override("sig_mvm_robot_limit_override");
		if (sig_mvm_robot_limit.GetBool()) {
			robots = sig_mvm_robot_limit_override.GetInt();
		}

		spectators = GetMaxSpectators();
		
		static ConVarRef sig_mvm_jointeam_blue_allow("sig_mvm_jointeam_blue_allow");
		static ConVarRef sig_mvm_jointeam_blue_allow_max("sig_mvm_jointeam_blue_allow_max");
		if (sig_mvm_jointeam_blue_allow.GetInt() != 0) {
			blu = sig_mvm_jointeam_blue_allow_max.GetInt() >= 0 ? sig_mvm_jointeam_blue_allow_max.GetInt() : gpGlobals->maxClients - robots - red - Max(spectators, 0);
        } 
		
		static ConVarRef sig_mvm_jointeam_blue_allow_force("sig_mvm_jointeam_blue_allow_force");
		if (sig_mvm_jointeam_blue_allow_force.GetInt() != 0 && sig_mvm_jointeam_blue_allow_max.GetInt() > 0) {
			blu = sig_mvm_jointeam_blue_allow_max.GetInt() >= 0 ? sig_mvm_jointeam_blue_allow_max.GetInt() : gpGlobals->maxClients - robots - Max(spectators, 0);
			red = 0;
		}

		static ConVarRef tv_enable("tv_enable");


		int freeSlots = MAX_PLAYERS - robots - tv_enable.GetBool() ? 1 : 0;
		if (spectators == -1) {
			spectators = Max(0, freeSlots - red - blu);
		}
		return red + blu + spectators + robots + tv_enable.GetBool() ? 1 : 0;
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
		
		bool ret = DETOUR_MEMBER_CALL(CTFGameRules_ClientConnected)(pEntity, pszName, pszAddress, reject, maxrejectlen);
        
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
		if (TFGameRules()->IsMannVsMachineMode() && pPlayer->IsRealPlayer()
			&& iWantedTeam == TF_TEAM_RED) {
				
			int totalPlayers = 0;
			ForEachTFPlayerOnTeam(TFTeamMgr()->GetTeam(TF_TEAM_RED), [&totalPlayers, pPlayer](CTFPlayer *player){
				if (player->IsRealPlayer() && pPlayer != player) {
					totalPlayers += 1;
				}
			});

			if (totalPlayers < iGetTeamAssignmentOverride) {
				Log( "MVM assigned %s to defending team (%d more slots remaining after us)\n", pPlayer->GetPlayerName(), iGetTeamAssignmentOverride - totalPlayers - 1 );
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
		auto iResult = DETOUR_MEMBER_CALL(CTFGameRules_GetTeamAssignmentOverride)(pPlayer, iWantedTeam, b1);
		
		return iResult;
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
            
            this->AddPatch(new CPatch_CTFGameRules_ClientConnected());
            this->AddPatch(new CPatch_CTFGCServerSystem_PreClientUpdate());
		}

		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

		virtual void LevelInitPostEntity() override
		{
            if (!TFGameRules()->IsMannVsMachineMode()) {
                allowspectators.Reset();
            }

			// Find a 10 players mvm plugin, and disable it, while also enabling 10 players here
			auto pluginIt = plsys->GetPluginIterator();
			while (pluginIt->MorePlugins()) {
				auto plugin = pluginIt->GetPlugin();
				if (FindCaseInsensitive(plugin->GetFilename(), "tf2mvm10plr") != nullptr) {
					plsys->UnloadPlugin(plugin);
					if (cvar_max_red_players.GetInt() == 0) {
						cvar_max_red_players.SetValue(10);
					}
					break;
				}
				
				pluginIt->NextPlugin();
			}
			pluginIt->Release();
            SetVisibleMaxPlayers();
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
								engine->ServerCommand(CFmtStr("kickid %d %s\n", player->GetUserID(), "Exceeded total player limit for the mission"));
								spectators -= 1;
							}
						}
					});
				}
				allowspectators.Set(spectators < sig_mvm_spectator_max_players.GetInt());
			}
        }
	};
	CMod s_Mod;

	ConVar cvar_enable("sig_mvm_player_limit_change", "0", FCVAR_NONE,
		"Mod: Change mvm player limits. Set to -1 to never allow missions or player limit convars to change this value",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetInt() > 0);
		});
	
	void TogglePatches() {
        if (s_Mod.IsEnabled()) {
            s_Mod.ToggleAllPatches(false);
            s_Mod.ToggleAllPatches(true);
        }
	}
}