#include "mod.h"
#include "stub/tfbot.h"
#include "util/scope.h"
#include "util/clientmsg.h"
#include "util/misc.h"
#include "stub/strings.h"
#include "stub/tfplayer.h"
#include "stub/gamerules.h"
#include "stub/misc.h"
#include "stub/server.h"
#include "stub/team.h"
#include "stub/tfweaponbase.h"
#include "stub/tf_player_resource.h"
#include "mod/pop/popmgr_extensions.h"
#include "util/iterate.h"
#include <gamemovement.h>

namespace Mod::Etc::Extra_Player_Slots
{
    ConVar sig_etc_extra_player_slots_count("sig_etc_extra_player_slots_count", "34", FCVAR_NOTIFY,
		"Extra player slot count. Requires map restart to function");

    ConVar sig_etc_extra_player_slots_allow_bots("sig_etc_extra_player_slots_allow_bots", "0", FCVAR_NOTIFY,
		"Allow bots to use extra player slots");

    ConVar sig_etc_extra_player_slots_voice_display_fix("sig_etc_extra_player_slots_voice_display_fix", "0", FCVAR_NOTIFY,
		"Fixes voice chat indicator showing with more than 64 slots, but also disables all of voice chat");

    ConVar sig_etc_extra_player_slots_no_death_cam("sig_etc_extra_player_slots_no_death_cam", "0", FCVAR_NOTIFY,
		"Does not display death cam when killed by players in extra slots");
    
    RefCount rc_CBaseServer_CreateFakeClient_HLTV;
    RefCount rc_CBaseServer_CreateFakeClient;

    inline bool ExtraSlotsEnabled()
    {
        return sig_etc_extra_player_slots_count.GetInt() > 33;
    }

    DETOUR_DECL_MEMBER(CBaseClient *, CBaseServer_CreateFakeClient, const char *name)
	{
        static ConVarRef tv_name("tv_name");
        static int counter = 0;
        SCOPED_INCREMENT(rc_CBaseServer_CreateFakeClient);
        SCOPED_INCREMENT_IF(rc_CBaseServer_CreateFakeClient_HLTV, tv_name.GetString() == name);
        if (tv_name.GetString() != name && strcmp(name, "TFBot") == 0) {
            name = STRING(AllocPooledString(CFmtStr("TFBot %d", ++counter)));
        }

        //ForEachEntity([](CBaseEntity *ent){
         //   Msg("Entity %d %s\n", ent->entindex(), ent->GetClassname());
		//	});
		return DETOUR_MEMBER_CALL(CBaseServer_CreateFakeClient)(name);
	}

    inline int GetHLTVSlot()
    {
        return 32;
    }

    int force_create_at_slot = -1;
    void SetForceCreateAtSlot(int slot) {
        force_create_at_slot = slot;
    }

    DETOUR_DECL_MEMBER(CBaseClient *, CBaseServer_GetFreeClient, netadr_t &adr)
	{
        auto server = reinterpret_cast<CBaseServer *>(this);
        if (server == hltv || ((!ExtraSlotsEnabled() || gpGlobals->maxClients < 34) && force_create_at_slot == -1)) return DETOUR_MEMBER_CALL(CBaseServer_GetFreeClient)(adr);

        if (rc_CBaseServer_CreateFakeClient || force_create_at_slot != -1) {
			static ConVarRef tv_enable("tv_enable");
            std::vector<CBaseClient *> clientList;

            // Make sure all slots are taken
            if (server->GetClientCount() != server->GetMaxClients()) {
                // Set clients as fake so they are considered taken
                for (int i = 0; i < server->GetClientCount(); i++) {
                    CBaseClient *client = static_cast<CBaseClient *>(server->GetClient(i));
                    if (!client->IsConnected() && !client->IsFakeClient()) {
                        clientList.push_back(client);
                        client->m_bFakePlayer = true;
                    }
                }

                // Create clients to fill all slots
                while (server->GetClientCount() != server->GetMaxClients()) {
                    CBaseClient *lastClient = DETOUR_MEMBER_CALL(CBaseServer_GetFreeClient)(adr);
                    if (lastClient != nullptr) {
                        clientList.push_back(lastClient);
                        lastClient->m_bFakePlayer = true;
                    }
                }
                for (auto client : clientList) {
                    client->m_bFakePlayer = false;
                }
            }
            
            if (force_create_at_slot != -1) {
                CBaseClient *client = static_cast<CBaseClient *>(server->GetClient(force_create_at_slot));
                force_create_at_slot = -1;
                return !client->IsConnected() && !client->IsFakeClient() ? client : nullptr;
            }

            int desiredSlot = GetHLTVSlot();
            if (!rc_CBaseServer_CreateFakeClient_HLTV) {
                if (!sig_etc_extra_player_slots_allow_bots.GetBool()) {
                    desiredSlot = 32;
                }
                else {
                    if (TFGameRules()->IsMannVsMachineMode()) {
                        static ConVarRef visible_max_players("sv_visiblemaxplayers");
                        static ConVarRef sig_mvm_robot_limit_override("sig_mvm_robot_limit_override");
                        int specs = MAX(0, Mod::Pop::PopMgr_Extensions::GetMaxSpectators());
                        desiredSlot = Clamp(visible_max_players.GetInt() + sig_mvm_robot_limit_override.GetInt() - 1 + specs + (tv_enable.GetBool() ? 1 : 0), 32, server->GetMaxClients() - 1);
                    }
                    else {
                        desiredSlot = gpGlobals->maxClients - 1;
                    }
                }
            }

            for (int i = desiredSlot; i >= 0; i--) {
                CBaseClient *client = static_cast<CBaseClient *>(server->GetClient(i));
                if (!client->IsConnected() 
                && !client->IsFakeClient() 
                && !(i == GetHLTVSlot() && !rc_CBaseServer_CreateFakeClient_HLTV && tv_enable.GetBool())) {
                    return client;
                }
            }

            return nullptr;
        }
		auto client = DETOUR_MEMBER_CALL(CBaseServer_GetFreeClient)(adr);
        if ( !(sig_etc_extra_player_slots_allow_bots.GetBool() && rc_CBaseServer_CreateFakeClient) && !rc_CBaseServer_CreateFakeClient_HLTV && client->GetPlayerSlot() > 32) {
            return nullptr;
        }
        if (!rc_CBaseServer_CreateFakeClient_HLTV && client->GetPlayerSlot() == gpGlobals->maxClients - 1) {
            return nullptr;
        }
        return client;
	}

    StaticFuncThunk<void, int> ft_SetupMaxPlayers("SetupMaxPlayers");
    //StaticFuncThunk<bool, bool, const char *, const char *> ft_Host_Changelevel("Host_Changelevel");
    //MemberFuncThunk<CBaseServer *, bool, const char *, const char *, const char *>              ft_SpawnServer("CGameServer::SpawnServer");

	DETOUR_DECL_MEMBER(void, CServerGameClients_GetPlayerLimits, int &minplayers, int &maxplayers, int &defaultplayers)
	{
		DETOUR_MEMBER_CALL(CServerGameClients_GetPlayerLimits)(minplayers,maxplayers,defaultplayers);
        if (ExtraSlotsEnabled())
		    maxplayers = sig_etc_extra_player_slots_count.GetInt();
	}

	DETOUR_DECL_MEMBER(void, CLagCompensationManager_FrameUpdatePostEntityThink)
	{
        int preMaxPlayers = gpGlobals->maxClients;

        if (preMaxPlayers > 33) {
            gpGlobals->maxClients = 33;
        }
		DETOUR_MEMBER_CALL(CLagCompensationManager_FrameUpdatePostEntityThink)();
        gpGlobals->maxClients = preMaxPlayers;
	}
	
	DETOUR_DECL_MEMBER(void, CLagCompensationManager_StartLagCompensation, CBasePlayer *player, CUserCmd *cmd)
	{
		int preMaxPlayers = gpGlobals->maxClients;

        if (preMaxPlayers > 33) {
            gpGlobals->maxClients = 33;
        }
		DETOUR_MEMBER_CALL(CLagCompensationManager_StartLagCompensation)(player, cmd);
        gpGlobals->maxClients = preMaxPlayers;
	}
	
	DETOUR_DECL_MEMBER(void, CLagCompensationManager_FinishLagCompensation, CBasePlayer *player)
	{
		int preMaxPlayers = gpGlobals->maxClients;

        if (preMaxPlayers > 33) {
            gpGlobals->maxClients = 33;
        }
		DETOUR_MEMBER_CALL(CLagCompensationManager_FinishLagCompensation)(player);
        gpGlobals->maxClients = preMaxPlayers;
	}

    int playerListOffset = 0;
	DETOUR_DECL_STATIC(void, SendProxy_PlayerList, const void *pProp, const void *pStruct, const void *pData, void *pOut, int iElement, int objectID)
	{
        if (!ExtraSlotsEnabled()) return DETOUR_STATIC_CALL(SendProxy_PlayerList)(pProp, pStruct, pData, pOut, iElement, objectID);
        if (iElement == 0) {
            playerListOffset = 0;
        }
        auto team = (CTeam *)(pData);
        int numplayers = team->GetNumPlayers();
        while (iElement + playerListOffset < numplayers - 1 && ENTINDEX(team->GetPlayer(iElement + playerListOffset)) > 33) {
            playerListOffset += 1;
        }
        //Msg("Element %d offset %d\n", iElement, playerListOffset);
        return DETOUR_STATIC_CALL(SendProxy_PlayerList)(pProp, pStruct, pData, pOut, iElement + playerListOffset, objectID);
    }
    DETOUR_DECL_STATIC(int, SendProxyArrayLength_PlayerArray, const void *pStruct, int objectID)
	{
		int count = DETOUR_STATIC_CALL(SendProxyArrayLength_PlayerArray)(pStruct, objectID);
        int countpre = count;
        if (ExtraSlotsEnabled()) {
            auto team = (CTeam *)(pStruct);
            
            //Msg("Send proxy Team is %d %d %d\n", team->GetTeamNumber(), count, team->GetNumPlayers());

            for (int i = 0; i < countpre; i++) {
                //Msg("Player %d\n", team->GetPlayer(i));
                if (ENTINDEX(team->GetPlayer(i)) > 33) {
                    count--;
                }
                //Msg("Player post\n");
            }
        }
        //Msg("Count %d Pre %d\n", count, countpre);
        return count;
    }

	DETOUR_DECL_MEMBER(void, CTeam_AddPlayer, CBasePlayer *player)
	{
        auto team = reinterpret_cast<CTFTeam *>(this);
        //Msg("Add plaayer Team is %d %d\n", team->GetTeamNumber(), player);
        if (ENTINDEX(player) < 34) {
            int insertindex = team->m_aPlayers->Count();
            while (insertindex > 0 && ENTINDEX(team->m_aPlayers[insertindex - 1]) > 33) {
                insertindex--;
            }
            team->m_aPlayers->InsertBefore(insertindex, player);
            team->NetworkStateChanged();
            return;
        }
        DETOUR_MEMBER_CALL(CTeam_AddPlayer)(player);

        // team->m_aPlayers->Sort([](CBasePlayer * const *l, CBasePlayer * const *r){
        //     return ENTINDEX(*l) - ENTINDEX(*r);
        // });
        //for (auto player : team->m_aPlayers) {
        //    Msg("team post %d\n", ENTINDEX(player));
        //}
    }
    DETOUR_DECL_MEMBER(void, CTFTeam_SetTeamLeader, CBasePlayer *player)
	{
        auto team = reinterpret_cast<CTFTeam *>(this);
        //if (ENTINDEX(player) > 33) {
        //    team->m_hLeader = nullptr;
        //    return;
        //}
        DETOUR_MEMBER_CALL(CTFTeam_SetTeamLeader)(player);
    }

	DETOUR_DECL_MEMBER(void, CTeam_RemovePlayer, CBasePlayer *player)
	{
        auto team = reinterpret_cast<CTFTeam *>(this);
        //Msg("Remove player Team is %d %d\n", team->GetTeamNumber(), player);
        //if (team->m_hLeader == player) {
        //    team->m_hLeader = nullptr;
        //}
        DETOUR_MEMBER_CALL(CTeam_RemovePlayer)(player);
    }

    DETOUR_DECL_MEMBER(void, CTFPlayer_UpdateOnRemove)
	{
        auto player = reinterpret_cast<CTFPlayer *>(this);
        DETOUR_MEMBER_CALL(CTFPlayer_UpdateOnRemove)();
        //Msg("Delete player %d %d\n", player->GetTeamNumber(), player);
    }

	DETOUR_DECL_MEMBER(int, CGameMovement_GetPointContentsCached, const Vector &point, int slot)
	{
        int oldIndex = -1;
        auto movement = reinterpret_cast<CGameMovement *>(this);
        if (movement->player != nullptr && ENTINDEX(movement->player) > 33) {
            oldIndex = ENTINDEX(movement->player);
            movement->player->edict()->m_EdictIndex = 32;
            //return enginetrace->GetPointContents(point);
        }
        auto ret = DETOUR_MEMBER_CALL(CGameMovement_GetPointContentsCached)(point, slot);
        if (oldIndex != -1) {
            movement->player->edict()->m_EdictIndex = oldIndex;
        }
        return ret;
    }

	DETOUR_DECL_MEMBER(int, CGameMovement_CheckStuck)
	{
        auto movement = reinterpret_cast<CGameMovement *>(this);
        int oldIndex = -1;
        if (movement->player != nullptr && ENTINDEX(movement->player) > 33) {
            oldIndex = ENTINDEX(movement->player);
            movement->player->edict()->m_EdictIndex = 0;
        }
        auto ret = DETOUR_MEMBER_CALL(CGameMovement_CheckStuck)();
        if (oldIndex != -1) {
            movement->player->edict()->m_EdictIndex = oldIndex;
        }
        return ret;
    }

	DETOUR_DECL_MEMBER(void, CTFGameStats_ResetPlayerStats, CTFPlayer* player) { if (ENTINDEX(player) > 33) return; DETOUR_MEMBER_CALL(CTFGameStats_ResetPlayerStats)(player);}
    DETOUR_DECL_MEMBER(void, CTFGameStats_ResetKillHistory, CTFPlayer* player) { if (ENTINDEX(player) > 33) return; DETOUR_MEMBER_CALL(CTFGameStats_ResetKillHistory)(player);}
    DETOUR_DECL_MEMBER(void, CTFGameStats_IncrementStat, CTFPlayer* player, int statType, int value) { if (ENTINDEX(player) > 33) return; DETOUR_MEMBER_CALL(CTFGameStats_IncrementStat)(player, statType, value);}
    DETOUR_DECL_MEMBER(void, CTFGameStats_SendStatsToPlayer, CTFPlayer* player, bool isAlive) { if (ENTINDEX(player) > 33) return; DETOUR_MEMBER_CALL(CTFGameStats_SendStatsToPlayer)(player, isAlive);}
    DETOUR_DECL_MEMBER(void, CTFGameStats_AccumulateAndResetPerLifeStats, CTFPlayer* player) { if (ENTINDEX(player) > 33) return; DETOUR_MEMBER_CALL(CTFGameStats_AccumulateAndResetPerLifeStats)(player);}
    DETOUR_DECL_MEMBER(void, CTFGameStats_Event_PlayerConnected, CTFPlayer* player) { if (ENTINDEX(player) > 33) return; DETOUR_MEMBER_CALL(CTFGameStats_Event_PlayerConnected)(player);}
    DETOUR_DECL_MEMBER(void, CTFGameStats_Event_PlayerDisconnectedTF, CTFPlayer* player) { if (ENTINDEX(player) > 33) return; DETOUR_MEMBER_CALL(CTFGameStats_Event_PlayerDisconnectedTF)(player);}
    DETOUR_DECL_MEMBER(void, CTFGameStats_Event_PlayerLeachedHealth, CTFPlayer* player, bool dispenser, float amount) { if (ENTINDEX(player) > 33) return; DETOUR_MEMBER_CALL(CTFGameStats_Event_PlayerLeachedHealth)(player, dispenser, amount);}
    DETOUR_DECL_MEMBER(void, CTFGameStats_TrackKillStats, CTFPlayer* attacker, CTFPlayer* victim) { if (ENTINDEX(attacker) > 33 || ENTINDEX(victim) > 33) return; DETOUR_MEMBER_CALL(CTFGameStats_TrackKillStats)(attacker, victim);}
    DETOUR_DECL_MEMBER(void *, CTFGameStats_FindPlayerStats, CTFPlayer* player) { if (ENTINDEX(player) > 33) return nullptr; return DETOUR_MEMBER_CALL(CTFGameStats_FindPlayerStats)(player);}
    DETOUR_DECL_MEMBER(void, CTFGameStats_Event_PlayerEarnedKillStreak, CTFPlayer* player) { if (ENTINDEX(player) > 33) return; DETOUR_MEMBER_CALL(CTFGameStats_Event_PlayerEarnedKillStreak)(player);}

    DETOUR_DECL_MEMBER(bool, CTFPlayerShared_IsPlayerDominated, int index) { if (index > 33) return false; return DETOUR_MEMBER_CALL(CTFPlayerShared_IsPlayerDominated)(index);}
    DETOUR_DECL_MEMBER(bool, CTFPlayerShared_IsPlayerDominatingMe, int index) { if (index > 33) return false; return DETOUR_MEMBER_CALL(CTFPlayerShared_IsPlayerDominatingMe)(index);}
    DETOUR_DECL_MEMBER(void, CTFPlayerShared_SetPlayerDominated, CTFPlayer * player, bool dominated) { if (ENTINDEX(player) > 33) return; DETOUR_MEMBER_CALL(CTFPlayerShared_SetPlayerDominated)(player, dominated);}
    DETOUR_DECL_MEMBER(void, CTFPlayerShared_SetPlayerDominatingMe, CTFPlayer * player, bool dominated) { if (ENTINDEX(player) > 33) return; DETOUR_MEMBER_CALL(CTFPlayerShared_SetPlayerDominatingMe)(player, dominated);}
    

    DETOUR_DECL_MEMBER(void, CTFPlayerResource_SetPlayerClassWhenKilled, int iIndex, int iClass )
	{
        if (iIndex > 33) {
            return;
        } 
        DETOUR_MEMBER_CALL(CTFPlayerResource_SetPlayerClassWhenKilled)(iIndex, iClass);
    }

    DETOUR_DECL_MEMBER(void, CBasePlayer_UpdatePlayerSound)
	{
        auto player = reinterpret_cast<CBasePlayer *>(this);
        if (player->entindex() > 96) return;
        DETOUR_MEMBER_CALL(CBasePlayer_UpdatePlayerSound)();
    }

    DETOUR_DECL_MEMBER(void, CSoundEnt_Initialize)
	{
        int oldMaxClients = gpGlobals->maxClients;
        if (gpGlobals->maxClients > 96) {
            gpGlobals->maxClients = 96;
        }
        DETOUR_MEMBER_CALL(CSoundEnt_Initialize)();
        gpGlobals->maxClients = oldMaxClients;
    }

    DETOUR_DECL_MEMBER(void, CVoiceGameMgr_ClientConnected, edict_t *edict)
	{
        if (ENTINDEX(edict) > 33) return;
        DETOUR_MEMBER_CALL(CVoiceGameMgr_ClientConnected)(edict);
    }

    DETOUR_DECL_MEMBER(void, CHLTVDirector_BuildActivePlayerList)
	{
        int oldMaxClients = gpGlobals->maxClients;
        if (gpGlobals->maxClients > 33) {
            gpGlobals->maxClients = 33;
        }
        DETOUR_MEMBER_CALL(CHLTVDirector_BuildActivePlayerList)();
        gpGlobals->maxClients = oldMaxClients;
    }

    DETOUR_DECL_MEMBER(void, CTFPlayer_HandleCommand_JoinTeam, const char * team)
	{
        char *ready = (char *)(TFGameRules()->m_bPlayerReady.Get());
        CTFPlayer *player = reinterpret_cast<CTFPlayer *>(this);
        char oldReady = ready[player->entindex()];
        Msg("pre ready %d \n", ready[player->entindex()]);
        DETOUR_MEMBER_CALL(CTFPlayer_HandleCommand_JoinTeam)(team);
        if (player->entindex() > 33) {
            Msg("post ready %d\n", ready[player->entindex()]);
            ready[player->entindex()] = oldReady;
        }
    }

    DETOUR_DECL_MEMBER(void, CTFGameRules_PlayerReadyStatus_UpdatePlayerState, CTFPlayer *pTFPlayer, bool bState)
	{
        if (ENTINDEX(pTFPlayer) > 33 ) return;

        DETOUR_MEMBER_CALL(CTFGameRules_PlayerReadyStatus_UpdatePlayerState)(pTFPlayer, bState);
    }

    DETOUR_DECL_MEMBER(void, CTFGCServerSystem_ClientDisconnected, CSteamID steamid)
	{
        int *state = (int *)(TFGameRules()->m_ePlayerWantsRematch.Get());
        int oldstate = -1;
        int index = ENTINDEX(UTIL_PlayerBySteamID(steamid));
        if (index > 33) {
            oldstate = state[index];
        }
        DETOUR_MEMBER_CALL(CTFGCServerSystem_ClientDisconnected)(steamid);
        if (oldstate != -1) {
            state[index] = oldstate;
        }
    }

    RefCount rc_CTFGameRules_ClientDisconnected;
    edict_t *disconnected_player_edict = nullptr;
    DETOUR_DECL_MEMBER(void, CTFGameRules_ClientDisconnected, edict_t *edict)
	{
		SCOPED_INCREMENT(rc_CTFGameRules_ClientDisconnected);
        disconnected_player_edict = edict;
		DETOUR_MEMBER_CALL(CTFGameRules_ClientDisconnected)(edict);
        disconnected_player_edict = nullptr;
	}

    DETOUR_DECL_MEMBER(int, CTFGameRules_GetTeamAssignmentOverride, CTFPlayer *pPlayer, int iWantedTeam, bool b1)
	{
		if (rc_CTFGameRules_ClientDisconnected && disconnected_player_edict != nullptr && iWantedTeam == 0 && pPlayer->entindex() == ENTINDEX(disconnected_player_edict)) {
            return 0;
        }
		return DETOUR_MEMBER_CALL(CTFGameRules_GetTeamAssignmentOverride)(pPlayer, iWantedTeam, b1);
	}

    DETOUR_DECL_MEMBER(bool, CTFGameMovement_CheckWater)
	{
		return false;//DETOUR_MEMBER_CALL(CTFGameMovement_CheckWater)();
	}

    CON_COMMAND(sig_get_entites, "")
    {
        ForEachEntity([](CBaseEntity *ent){
            Msg("Entity %d %s\n", ent->entindex(), ent->GetClassname());
            ent->PostClientActive();
			});
    }

    CON_COMMAND(sig_get_commands, "")
    {
        const ConCommandBase *cmd = icvar->GetCommands();
        for ( ; cmd; cmd = cmd->GetNext() )
        {
            Msg("%s\n", cmd->GetName());
        }
    }

    CON_COMMAND(sig_list_team, "")
    {
        for (int i = 0; i < 4; i++) {
            auto team = TFTeamMgr()->GetTeam(i);
            Msg("Team %d:\n", i);
            if (team != nullptr) {
                for (int j = 0; j < team->GetNumPlayers(); j++) {
                    Msg(" player %d\n", team->GetPlayer(j));
                }
            }
        }
    }

    DETOUR_DECL_MEMBER(ConVar *, CCvar_FindVar, const char *name)
	{
        Msg("CVar %s\n", name);
        return DETOUR_MEMBER_CALL(CCvar_FindVar)(name);
    }

    DETOUR_DECL_MEMBER(const char *, ConVar_GetName)
	{
        auto ret = DETOUR_MEMBER_CALL(ConVar_GetName)();
        
        Msg("CVarn %s\n", ret);
        return ret;
    }
    DETOUR_DECL_MEMBER(const char *, ConCommandBase_GetName)
	{
        auto ret = DETOUR_MEMBER_CALL(ConCommandBase_GetName)();
        
        Msg("CCmd %s\n", ret);
        return ret;
    }

    float talk_time[64];
    
    class CVoiceGameMgr {};
    MemberFuncThunk<CVoiceGameMgr *, void>  ft_CVoiceGameMgr_UpdateMasks("CVoiceGameMgr::UpdateMasks");
    void *voicemgr = nullptr;
    DETOUR_DECL_STATIC(bool, SV_BroadcastVoiceData, IClient * pClient, int nBytes, char * data, int64 xuid)
    {
        if (gpGlobals->maxClients > 64 && pClient->GetPlayerSlot() < 64) {
            // Force update player voice masks
            bool update = talk_time[pClient->GetPlayerSlot()] < gpGlobals->curtime - 1;
            talk_time[pClient->GetPlayerSlot()] = gpGlobals->curtime;
            if (update && voicemgr != nullptr) {
                ft_CVoiceGameMgr_UpdateMasks((CVoiceGameMgr *)voicemgr);
            }
        }    
        return DETOUR_STATIC_CALL(SV_BroadcastVoiceData)(pClient, nBytes, data, xuid);
    }

    DETOUR_DECL_MEMBER(bool, CVoiceGameMgrHelper_CanPlayerHearPlayer, CBasePlayer *pListener, CBasePlayer *pTalker, bool &bProximity)
	{
        if (gpGlobals->maxClients > 64 && sig_etc_extra_player_slots_voice_display_fix.GetBool() 
            && (ENTINDEX(pTalker) > 64 || talk_time[ENTINDEX(pTalker) - 1] + 1.0f < gpGlobals->curtime)) {
            //Msg("talker %d %d\n", ENTINDEX(pTalker) , (ENTINDEX(pTalker) > 64 || talk_time[ENTINDEX(pTalker) - 1] + 1.0f > gpGlobals->curtime));
                return false;}

        return DETOUR_MEMBER_CALL(CVoiceGameMgrHelper_CanPlayerHearPlayer)(pListener, pTalker, bProximity);
    }

    DETOUR_DECL_MEMBER(void, CVoiceGameMgr_UpdateMasks)
	{
        voicemgr = this;
        static ConVarRef sv_alltalk("sv_alltalk");
        if (gpGlobals->maxClients > 64 && sig_etc_extra_player_slots_voice_display_fix.GetBool() && sv_alltalk.GetBool()) {
            sv_alltalk.SetValue(false);
        }

        DETOUR_MEMBER_CALL(CVoiceGameMgr_UpdateMasks)();
        
        // if (restoreAllTalk) {
        //     sv_alltalk.SetValue(true);
        // }
    }

    DETOUR_DECL_MEMBER(void, CTeamplayRoundBasedRules_State_Enter_PREROUND)
	{
        // Prematurely send bots to spawn. This way you can avoid edict limit being hit during map reload
        if (TFGameRules()->IsMannVsMachineMode()) {
            ForEachTFBot([](CTFBot *bot){
                if (bot->GetTeamNumber() > TEAM_SPECTATOR) {
                    bot->ChangeTeam(TEAM_SPECTATOR, false, true, false);
                }
            });
        }
        DETOUR_MEMBER_CALL(CTeamplayRoundBasedRules_State_Enter_PREROUND)();
    }

    DETOUR_DECL_MEMBER(void, CTFGameRules_PowerupModeKillCountCompare)
	{
        Msg("called kill count compare\n");
        DETOUR_MEMBER_CALL(CTFGameRules_PowerupModeKillCountCompare)();
    }

    DETOUR_DECL_MEMBER(void, CTFGameRules_PowerupModeInitKillCountTimer)
	{
        Msg("called init kill count timer\n");
        DETOUR_MEMBER_CALL(CTFGameRules_PowerupModeInitKillCountTimer)();
    }

    struct KillEvent
    {
        float time;
        IGameEvent *event;
        bool send = false;
        int fakePlayersIndex[3] {-1,-1,-1};
        std::string fakePlayersOldNames[3];
        int fakePlayersTeams[3] {-1, -1, -1};
    };
    std::deque<KillEvent> kill_events;
    int player33_fake_team_num=-1;
    float player33_fake_kill_time=FLT_MIN;
    
    CBasePlayer *FindFreeFakePlayer(std::vector<CBasePlayer *> &checkVec) 
    {
        for (int i = 33; i >= 1; i--) {
            bool found = false;

            auto playeri = UTIL_PlayerByIndex(i);
            if (playeri == nullptr) continue;

            if (!playeri->IsHLTV() && !playeri->IsBot()) continue;

            for (auto &event : kill_events) {
                for (auto index : event.fakePlayersIndex) {
                    if (index == i) {
                        found = true;
                        break;
                    }
                }
                if (found) break;
            }
            if (found) continue;

            for (auto player : checkVec) {
                if (ENTINDEX(player) == i) {
                    found = true;
                    break;
                }
            }
            if (found) continue;

            return playeri;
        }

        return UTIL_PlayerByIndex(33);
    }

    MemberFuncThunk<CBaseClient *, void, IGameEvent *>  ft_CBaseClient_FireGameEvent("CBaseClient::FireGameEvent");

    bool sending_delayed_event = false;
    DETOUR_DECL_MEMBER(void, CBaseClient_FireGameEvent, IGameEvent *event)
	{
        // Replace extra slot client id with some other bot with lower id
        IGameEvent *duplicate = nullptr;
        const char *eventName = nullptr;
        if (!sending_delayed_event && event != nullptr && sig_etc_extra_player_slots_allow_bots.GetBool() ) {
            eventName = event->GetName();
        }
        if ( eventName != nullptr && (strcmp(eventName, "player_death") == 0 || strcmp(eventName, "object_destroyed") == 0
              || StringStartsWith(eventName, "fish_notice"))) {
            auto victim = UTIL_PlayerByUserId(event->GetInt("userid"));
            auto attacker = UTIL_PlayerByUserId(event->GetInt("attacker"));
            auto assister = UTIL_PlayerByUserId(event->GetInt("assister"));
            auto player33 = UTIL_PlayerByIndex(33);
            if (ENTINDEX(victim) > 33 || ENTINDEX(attacker) > 33 || ENTINDEX(assister) > 33) {
                std::vector<CBasePlayer *> participants;
                std::vector<CBasePlayer *> fakePlayers;
                duplicate = gameeventmanager->DuplicateEvent(event);
                if (ENTINDEX(victim) > 33) {
                    auto fakePlayer = FindFreeFakePlayer(fakePlayers);
                    duplicate->SetInt("userid", fakePlayer->GetUserID());
                    fakePlayers.push_back(fakePlayer);
                    participants.push_back(victim);
                }
                if (ENTINDEX(attacker) > 33) {
                    auto fakePlayer = FindFreeFakePlayer(fakePlayers);
                    duplicate->SetInt("attacker", fakePlayer->GetUserID());
                    fakePlayers.push_back(fakePlayer);
                    participants.push_back(attacker);
                }
                if (ENTINDEX(assister) > 33) {
                    auto fakePlayer = FindFreeFakePlayer(fakePlayers);
                    duplicate->SetInt("assister", fakePlayer->GetUserID());
                    fakePlayers.push_back(fakePlayer);
                    participants.push_back(assister);
                }
                //auto fakePlayer = nullptr;
                /*for (int i = 33; i >= 1; i--) {
                    auto player = UTIL_PlayerByIndex(i);
                    if (player != nullptr && player->GetTeamNumber() == copyNameFromPlayer->GetTeamNumber() && player->GetName()) {
                        
                    }
                }*/
                //gameeventmanager->SerializeEvent(duplicate);
                kill_events.push_back({gpGlobals->curtime, duplicate});
                for (size_t i = 0; i < participants.size(); i++) {
                    kill_events.back().fakePlayersOldNames[i] = fakePlayers[i]->GetPlayerName();
                    kill_events.back().fakePlayersIndex[i] = ENTINDEX(fakePlayers[i]);
                    kill_events.back().fakePlayersTeams[i] = participants[i]->GetTeamNumber();
                    fakePlayers[i]->SetPlayerName(participants[i]->GetPlayerName());
                    engine->SetFakeClientConVarValue(fakePlayers[i]->edict(), "name", participants[i]->GetPlayerName());
                    TFPlayerResource()->m_iTeam.SetIndex(participants[i]->GetTeamNumber(), fakePlayers[i]->entindex());
                }
                player33_fake_kill_time = gpGlobals->curtime;
                //player33_fake_team_num = copyNameFromPlayer->GetTeamNumber();
                return;
                //event = duplicate;
            }
        }
        auto client = reinterpret_cast<CBaseClient *>(this);
        DETOUR_MEMBER_CALL(CBaseClient_FireGameEvent)(event);
    }

    DETOUR_DECL_STATIC(void, SV_ComputeClientPacks, int clientCount,  void **clients, void *snapshot)
	{
        int realTeam = -1;
		if (player33_fake_kill_time + 1 > gpGlobals->curtime) {

            for (auto it = kill_events.begin(); it != kill_events.end();){
                auto &killEvent = (*it);
                
                if (killEvent.time + 0.2f < gpGlobals->curtime && !killEvent.send) {
                    killEvent.send = true;
                    sending_delayed_event = true;
                    for (int i = 0; i < sv->GetMaxClients(); i++) {
                        auto client = static_cast<CBaseClient *>(sv->GetClient(i));
                        if (client != nullptr && client->IsConnected() && !client->IsFakeClient()) {
                            ft_CBaseClient_FireGameEvent(client, killEvent.event);
                        }
                    }
                    sending_delayed_event = false;

                    gameeventmanager->FreeEvent(killEvent.event);
                }

                for (int i = 0; i < 3; i++) {
                    if (killEvent.fakePlayersIndex[i] != -1) {
                        TFPlayerResource()->m_iTeam.SetIndex(killEvent.fakePlayersTeams[i], killEvent.fakePlayersIndex[i]);
                    }
                }

                if (killEvent.time + 0.35f < gpGlobals->curtime) {
                    
                    for (int i = 0; i < 3; i++) {
                        if (killEvent.fakePlayersIndex[i] != -1 && killEvent.fakePlayersIndex[i] != 33) {
                            
                            auto player = UTIL_PlayerByIndex(killEvent.fakePlayersIndex[i]);
                            if (player != nullptr)
                                player->SetPlayerName(killEvent.fakePlayersOldNames[i].c_str());
                            engine->SetFakeClientConVarValue(INDEXENT(killEvent.fakePlayersIndex[i]), "name", killEvent.fakePlayersOldNames[i].c_str());
                        }
                    }

                    it = kill_events.erase(it);
                }
                else {
                    it++;
                }
            }
        }
        //if (player33_fake_kill_time + 2 > gpGlobals->curtime) {
        //    realTeam = TFPlayerResource()->m_iTeam[33];
        //}
		DETOUR_STATIC_CALL(SV_ComputeClientPacks)(clientCount, clients, snapshot);
        //if (realTeam != -1) {
//
        //    TFPlayerResource()->m_iTeam.SetIndex(realTeam, 33);
        //}
	}

    DETOUR_DECL_MEMBER(void, CSteam3Server_SendUpdatedServerDetails)
	{
        //int &players = static_cast<CBaseServer *>(sv)->GetMaxClientsRef();
        //int oldPlayers = players;
        //if (players > 33) {
        //    players = 33;
        //}
        static ConVarRef sv_visiblemaxplayers("sv_visiblemaxplayers");
        int oldsv_visiblemaxplayers = -1; 
        if (sv_visiblemaxplayers.GetInt() > 33) {
            oldsv_visiblemaxplayers = sv_visiblemaxplayers.GetInt();
            sv_visiblemaxplayers.SetValue(33);
        }
        DETOUR_MEMBER_CALL(CSteam3Server_SendUpdatedServerDetails)();
        if (oldsv_visiblemaxplayers != -1) {
            sv_visiblemaxplayers.SetValue(oldsv_visiblemaxplayers);
        }
        //players = oldPlayers;
    }

    DETOUR_DECL_MEMBER(void, CTFGameRules_CalcDominationAndRevenge, CTFPlayer *pAttacker, CBaseEntity *pWeapon, CTFPlayer *pVictim, bool bIsAssist, int *piDeathFlags)
	{
        if (ENTINDEX(pAttacker) > 33 || ENTINDEX(pVictim) > 33) return;

        DETOUR_MEMBER_CALL(CTFGameRules_CalcDominationAndRevenge)(pAttacker, pWeapon, pVictim, bIsAssist, piDeathFlags);
    }



    DETOUR_DECL_MEMBER(const char *, CTFBot_GetNextSpawnClassname)
	{
        auto bot = reinterpret_cast<CTFBot *>(this);
        auto team = TFTeamMgr()->GetTeam(bot->GetTeamNumber());
        if (team != nullptr && team->GetNumPlayers() >= 16) {
            return g_aRawPlayerClassNames[(ENTINDEX(bot)/2 % 9) + 1];
        }
		return DETOUR_MEMBER_CALL(CTFBot_GetNextSpawnClassname)();
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_Event_Killed, const CTakeDamageInfo& info)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		DETOUR_MEMBER_CALL(CTFPlayer_Event_Killed)(info);
        if (sig_etc_extra_player_slots_no_death_cam.GetBool() && ToTFPlayer(player->m_hObserverTarget) != nullptr && ENTINDEX(player->m_hObserverTarget) > 33) {
            player->m_hObserverTarget = nullptr;
        }
	}
    
	class CMod : public IMod, public IModCallbackListener
	{
	public:
		CMod() : IMod("Perf:Extra_Player_Slots")
		{

			MOD_ADD_DETOUR_MEMBER(CBaseServer_CreateFakeClient,              "CBaseServer::CreateFakeClient");
			MOD_ADD_DETOUR_MEMBER(CBaseServer_GetFreeClient,              "CBaseServer::GetFreeClient");
			MOD_ADD_DETOUR_MEMBER(CServerGameClients_GetPlayerLimits, "CServerGameClients::GetPlayerLimits");
			MOD_ADD_DETOUR_MEMBER(CLagCompensationManager_FrameUpdatePostEntityThink, "CLagCompensationManager::FrameUpdatePostEntityThink");
            MOD_ADD_DETOUR_MEMBER(CLagCompensationManager_StartLagCompensation,  "CLagCompensationManager::StartLagCompensation");
			MOD_ADD_DETOUR_MEMBER(CLagCompensationManager_FinishLagCompensation, "CLagCompensationManager::FinishLagCompensation");
			//MOD_ADD_DETOUR_STATIC(SendProxy_PlayerList,    "SendProxy_PlayerList");
		    MOD_ADD_DETOUR_STATIC(SendProxyArrayLength_PlayerArray,    "SendProxyArrayLength_PlayerArray");
			MOD_ADD_DETOUR_MEMBER(CTeam_AddPlayer, "CTeam::AddPlayer");
			//MOD_ADD_DETOUR_MEMBER(CTeam_RemovePlayer, "CTeam::RemovePlayer");
			MOD_ADD_DETOUR_MEMBER(CGameMovement_GetPointContentsCached, "CGameMovement::GetPointContentsCached");

            
			MOD_ADD_DETOUR_MEMBER(CTFGameStats_ResetPlayerStats, "CTFGameStats::ResetPlayerStats");
            MOD_ADD_DETOUR_MEMBER(CTFGameStats_ResetKillHistory, "CTFGameStats::ResetKillHistory");
            MOD_ADD_DETOUR_MEMBER(CTFGameStats_IncrementStat, "CTFGameStats::IncrementStat");
            MOD_ADD_DETOUR_MEMBER(CTFGameStats_SendStatsToPlayer, "CTFGameStats::SendStatsToPlayer");
            MOD_ADD_DETOUR_MEMBER(CTFGameStats_AccumulateAndResetPerLifeStats, "CTFGameStats::AccumulateAndResetPerLifeStats");
            MOD_ADD_DETOUR_MEMBER(CTFGameStats_Event_PlayerConnected, "CTFGameStats::Event_PlayerConnected");
            MOD_ADD_DETOUR_MEMBER(CTFGameStats_Event_PlayerDisconnectedTF, "CTFGameStats::Event_PlayerDisconnectedTF");
            MOD_ADD_DETOUR_MEMBER(CTFGameStats_Event_PlayerLeachedHealth, "CTFGameStats::Event_PlayerLeachedHealth");
            MOD_ADD_DETOUR_MEMBER(CTFGameStats_TrackKillStats, "CTFGameStats::TrackKillStats");
            MOD_ADD_DETOUR_MEMBER(CTFGameStats_FindPlayerStats, "CTFGameStats::FindPlayerStats");
            MOD_ADD_DETOUR_MEMBER(CTFGameStats_Event_PlayerEarnedKillStreak, "CTFGameStats::Event_PlayerEarnedKillStreak");

            MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_IsPlayerDominated, "CTFPlayerShared::IsPlayerDominated");
            MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_IsPlayerDominatingMe, "CTFPlayerShared::IsPlayerDominatingMe");
            MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_SetPlayerDominated, "CTFPlayerShared::SetPlayerDominated");
            MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_SetPlayerDominatingMe, "CTFPlayerShared::SetPlayerDominatingMe");
            MOD_ADD_DETOUR_MEMBER(CTFPlayerResource_SetPlayerClassWhenKilled, "CTFPlayerResource::SetPlayerClassWhenKilled");
            MOD_ADD_DETOUR_MEMBER(CBasePlayer_UpdatePlayerSound, "CBasePlayer::UpdatePlayerSound");
            MOD_ADD_DETOUR_MEMBER(CSoundEnt_Initialize, "CSoundEnt::Initialize");
            MOD_ADD_DETOUR_MEMBER(CVoiceGameMgr_ClientConnected, "CVoiceGameMgr::ClientConnected");
            //MOD_ADD_DETOUR_MEMBER(CTFTeam_SetTeamLeader, "CTFTeam::SetTeamLeader");
            MOD_ADD_DETOUR_MEMBER(CHLTVDirector_BuildActivePlayerList, "CHLTVDirector::BuildActivePlayerList");
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_HandleCommand_JoinTeam, "CTFPlayer::HandleCommand_JoinTeam");
            MOD_ADD_DETOUR_MEMBER(CTFGameRules_PlayerReadyStatus_UpdatePlayerState, "CTFGameRules::PlayerReadyStatus_UpdatePlayerState");
            MOD_ADD_DETOUR_MEMBER(CTFGCServerSystem_ClientDisconnected, "CTFGCServerSystem::ClientDisconnected");
            //MOD_ADD_DETOUR_MEMBER(CTFPlayer_UpdateOnRemove, "CTFPlayer::UpdateOnRemove");
            MOD_ADD_DETOUR_MEMBER(CTFGameRules_ClientDisconnected, "CTFGameRules::ClientDisconnected");
            MOD_ADD_DETOUR_MEMBER(CTFGameRules_GetTeamAssignmentOverride, "CTFGameRules::GetTeamAssignmentOverride");
            //MOD_ADD_DETOUR_MEMBER(ConVar_GetName, "ConVar::GetName");
            //MOD_ADD_DETOUR_MEMBER(ConCommandBase_GetName, "ConCommandBase::GetName");
            MOD_ADD_DETOUR_STATIC(SV_BroadcastVoiceData, "SV_BroadcastVoiceData");
            MOD_ADD_DETOUR_MEMBER(CVoiceGameMgrHelper_CanPlayerHearPlayer, "CVoiceGameMgrHelper::CanPlayerHearPlayer");
            MOD_ADD_DETOUR_MEMBER(CVoiceGameMgr_UpdateMasks, "CVoiceGameMgr::UpdateMasks");
            MOD_ADD_DETOUR_MEMBER(CTeamplayRoundBasedRules_State_Enter_PREROUND, "CTeamplayRoundBasedRules::State_Enter_PREROUND");
            //MOD_ADD_DETOUR_MEMBER(CTFGameRules_PowerupModeKillCountCompare, "CTFGameRules::PowerupModeKillCountCompare");
            //MOD_ADD_DETOUR_MEMBER(CTFGameRules_PowerupModeInitKillCountTimer, "CTFGameRules::PowerupModeInitKillCountTimer");
            
            MOD_ADD_DETOUR_MEMBER(CBaseClient_FireGameEvent, "CBaseClient::FireGameEvent");
            MOD_ADD_DETOUR_MEMBER(CGameMovement_CheckStuck, "CGameMovement::CheckStuck");
            MOD_ADD_DETOUR_MEMBER(CSteam3Server_SendUpdatedServerDetails, "CSteam3Server::SendUpdatedServerDetails");
            MOD_ADD_DETOUR_MEMBER(CTFGameRules_CalcDominationAndRevenge, "CTFGameRules::CalcDominationAndRevenge");
            
            MOD_ADD_DETOUR_STATIC(SV_ComputeClientPacks, "SV_ComputeClientPacks");
            MOD_ADD_DETOUR_MEMBER(CTFBot_GetNextSpawnClassname, "CTFBot::GetNextSpawnClassname");
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_Event_Killed, "CTFPlayer::Event_Killed");
            
            
            
			//MOD_ADD_DETOUR_MEMBER(CTFPlayer_ShouldTransmit,               "CTFPlayer::ShouldTransmit");
            //MOD_ADD_DETOUR_STATIC(SendTable_CalcDelta,   "SendTable_CalcDelta");
		}

		virtual void PreLoad() override
		{
            //this->AddDetour(new CDetour("CCvar::FindVar", (void *)(LibMgr::GetInfo(Library::VSTDLIB).BaseAddr() + 0x0010470), GET_MEMBER_CALLBACK(CCvar_FindVar), GET_MEMBER_INNERPTR(CCvar_FindVar)));
		}

        virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

        virtual void OnEnablePost() override
		{

        }

        virtual void LevelInitPreEntity() override
		{
            player33_fake_team_num = -1;
            player33_fake_kill_time=FLT_MIN;
            kill_events.clear();

            if (ExtraSlotsEnabled() && (gpGlobals->maxClients >= 32 && gpGlobals->maxClients < sig_etc_extra_player_slots_count.GetInt())) {
                engine->ChangeLevel(STRING(gpGlobals->mapname), nullptr);
            }
            static bool saved_old_alltalk = false;
            static bool old_alltalk = false;
            static ConVarRef sv_alltalk("sv_alltalk");
            if (gpGlobals->maxClients > 64 && sig_etc_extra_player_slots_voice_display_fix.GetBool()) {
                saved_old_alltalk = true;
                old_alltalk = sv_alltalk.GetBool();
            }
            else if (saved_old_alltalk) {
                saved_old_alltalk = false;
                sv_alltalk.SetValue(old_alltalk);
            }
            for (auto &time : talk_time) {
                time = -100.0f;
            }
        }

        virtual void LevelShutdownPostEntity() override
		{

            if (ExtraSlotsEnabled() && (gpGlobals->maxClients >= 32 && gpGlobals->maxClients < sig_etc_extra_player_slots_count.GetInt())) {
                // Kick old HLTV client
                int clientCount = sv->GetClientCount();
                for ( int i=0 ; i < clientCount ; i++ )
                {
                    IClient *pClient = sv->GetClient( i );

                    if (pClient->IsConnected() && pClient->IsHLTV() && i <= 33)
                    {
                        pClient->Disconnect("");
                        break;
                    }
                }
                ft_SetupMaxPlayers(sig_etc_extra_player_slots_count.GetInt());
            }
        }
	};
	CMod s_Mod;
    
	ConVar cvar_enable("sig_etc_extra_player_slots", "0", FCVAR_NOTIFY,
		"Mod: allows usage of extra player slots",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}