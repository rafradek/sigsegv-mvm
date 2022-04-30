#include "mod.h"
#include "util/scope.h"
#include "util/clientmsg.h"
#include "util/misc.h"
#include "util/iterate.h"
#include "stub/tfplayer.h"
#include "stub/gamerules.h"
#include "stub/misc.h"
#include "stub/server.h"
#include "stub/team.h"
#include "stub/tfweaponbase.h"
#include "stub/tf_player_resource.h"
#include <gamemovement.h>

namespace Mod::Perf::HLTV_Optimize
{

    ConVar cvar_rate_between_rounds("sig_perf_hltv_rate_between_rounds", "8", FCVAR_NOTIFY,
		"Source TV snapshotrate between rounds");

    bool hltvServerEmpty = false;
    bool recording = false;

    DETOUR_DECL_MEMBER(bool, CHLTVServer_IsTVRelay)
	{
        return false;
    }

    float old_snapshotrate = 16.0f;
    RefCount rc_CHLTVServer_RunFrame;
	DETOUR_DECL_MEMBER(void, CHLTVServer_RunFrame)
	{
        SCOPED_INCREMENT(rc_CHLTVServer_RunFrame);
        CHLTVServer *hltvserver = reinterpret_cast<CHLTVServer *>(this);
        
        static ConVarRef tv_enable("tv_enable");
        static ConVarRef tv_maxclients("tv_maxclients");
        bool tv_enabled = tv_enable.GetBool();

        hltvServerEmpty = hltvserver->GetNumClients() == 0;

        bool hasplayer = tv_maxclients.GetInt() != 0;
        IClient * hltvclient = nullptr;

        int clientCount = sv->GetClientCount();
        for ( int i=0 ; i < clientCount ; i++ )
        {
            IClient *pClient = sv->GetClient( i );

            if ( pClient->IsConnected() )
            {
                if (!hasplayer && tv_enabled && !pClient->IsFakeClient() && !pClient->IsHLTV()) {
                    hasplayer = true;
                }
                else if (hltvclient == nullptr && pClient->IsHLTV()) {
                    hltvclient = pClient;
                }
            }
            if ((hasplayer || !tv_enabled) && hltvclient != nullptr)
                break;
        }

        if (!hasplayer && hltvclient != nullptr) {
            hltvclient->Disconnect("");
        }

        if (hasplayer && hltvclient == nullptr && sv->IsActive()) {
            DevMsg("spawning sourcetv\n");
            static ConVarRef tv_name("tv_name");
            CBaseClient *client = reinterpret_cast<CBaseServer *>(sv)->CreateFakeClient(tv_name.GetString());
            if (client != nullptr) {
                DevMsg("spawning sourcetv client %d\n", client->GetPlayerSlot());
                reinterpret_cast<CHLTVServer *>(this)->StartMaster(client);
            }
        }
        if (hasplayer && hltvclient != nullptr) {
            static ConVarRef snapshotrate("tv_snapshotrate");
            if (hltvServerEmpty) {
                int tickcount = 32.0f / (snapshotrate.GetFloat() * gpGlobals->interval_per_tick);
                int framec = hltvserver->CountClientFrames() - 2;
                for (int i = 0; i < framec; i++)
                    hltvserver->RemoveOldestFrame();
                //DevMsg("SendNow %d\n", gpGlobals->tickcount % tickcount == 0/*reinterpret_cast<CGameClient *>(hltvclient)->ShouldSendMessages()*/);
                if (gpGlobals->tickcount % tickcount != 0)
                    return;
            }
        }
		DETOUR_MEMBER_CALL(CHLTVServer_RunFrame)();
	}

    DETOUR_DECL_MEMBER(void, CHLTVServer_UpdateTick)
	{
        if (!hltvServerEmpty) {
            CHLTVServer *hltvserver = reinterpret_cast<CHLTVServer *>(this);
            static ConVarRef snapshotrate("tv_snapshotrate");
            static ConVarRef delay("tv_delay");
            int tickcount = 1.0f / (snapshotrate.GetFloat() * gpGlobals->interval_per_tick);
            if (delay.GetInt() <= 0) {
                int framec = hltvserver->CountClientFrames() - 2;
                for (int i = 0; i < framec; i++)
                    hltvserver->RemoveOldestFrame();
            }
            //DevMsg("SendNow %d\n", gpGlobals->tickcount % tickcount == 0/*reinterpret_cast<CGameClient *>(hltvclient)->ShouldSendMessages()*/);
            if (gpGlobals->tickcount % tickcount != 0)
                return;
        }
		DETOUR_MEMBER_CALL(CHLTVServer_UpdateTick)();
    }

    int last_restore_tick = -1;
    //Do not delay stringtables in demo recording
    DETOUR_DECL_MEMBER(void, CHLTVServer_RestoreTick, int time)
	{
        //static ConVarRef delay("tv_delay");
        //if (hltvServerEmpty)
            //return;
        CFastTimer timer;
        timer.Start();
        DETOUR_MEMBER_CALL(CHLTVServer_RestoreTick)(time);
        timer.End();
        last_restore_tick = time;
    }

    //Do not delay stringtables in demo recording

    DETOUR_DECL_MEMBER(void, CNetworkStringTable_RestoreTick, int tick)
	{
        auto table = reinterpret_cast<CNetworkStringTable *>(this);
        int last_change = table->m_nLastChangedTick;

        if (tick == 0 || (last_change > last_restore_tick && last_change <= tick)) {
            DETOUR_MEMBER_CALL(CNetworkStringTable_RestoreTick)(tick);
            table->m_nLastChangedTick = last_change;
        }
        
    }

    
    DETOUR_DECL_MEMBER(void, CNetworkStringTable_UpdateMirrorTable, int tick)
    {
        DETOUR_MEMBER_CALL(CNetworkStringTable_UpdateMirrorTable)(tick);
        auto table = reinterpret_cast<CNetworkStringTable *>(this);
        
        if (table->m_pMirrorTable != nullptr) {
            table->m_pMirrorTable->m_nLastChangedTick = tick;
        }
    }
	DETOUR_DECL_MEMBER(void, CTeamplayRoundBasedRules_State_Enter, gamerules_roundstate_t newState)
	{
        static ConVarRef snapshotrate("tv_snapshotrate");
        if (newState != GR_STATE_RND_RUNNING && hltvServerEmpty) {
            if (snapshotrate.GetFloat() != cvar_rate_between_rounds.GetFloat()) {
                old_snapshotrate = snapshotrate.GetFloat();
                snapshotrate.SetValue(cvar_rate_between_rounds.GetFloat());
            }
        }
        else
        {
            if (snapshotrate.GetFloat() == cvar_rate_between_rounds.GetFloat()) {
                snapshotrate.SetValue(old_snapshotrate);
            }
        }
        DETOUR_MEMBER_CALL(CTeamplayRoundBasedRules_State_Enter)(newState);
    }

	DETOUR_DECL_MEMBER(void, NextBotPlayer_CTFPlayer_PhysicsSimulate)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (player->GetTeamNumber() <= TEAM_SPECTATOR && !player->IsAlive() && gpGlobals->curtime - player->GetDeathTime() > 1.0f && gpGlobals->tickcount % 64 != 0)
			return;
            
		DETOUR_MEMBER_CALL(NextBotPlayer_CTFPlayer_PhysicsSimulate)();
	}

    DETOUR_DECL_MEMBER(void, CBasePlayer_PhysicsSimulate)
	{
		auto player = reinterpret_cast<CBasePlayer *>(this);

		if (player->IsHLTV()) {
			return;
        }
            
		DETOUR_MEMBER_CALL(CBasePlayer_PhysicsSimulate)();
	}

    ConVar sig_perf_hltv_use_special_slot("sig_perf_hltv_use_special_slot", "34", FCVAR_NOTIFY,
		"Use special slot 34 for hltv. Requires map restart to function");

    ConVar sig_perf_hltv_allow_bots_extra_slot("sig_perf_hltv_allow_bots_extra_slot", "0", FCVAR_NOTIFY,
		"Use special slot 34 for hltv. Requires map restart to function");
    
    RefCount rc_CBaseServer_CreateFakeClient_HLTV;
    RefCount rc_CBaseServer_CreateFakeClient;

    inline bool ExtraSlotsEnabled()
    {
        return sig_perf_hltv_use_special_slot.GetInt() > 33;
    }

    DETOUR_DECL_MEMBER(CBaseClient *, CBaseServer_CreateFakeClient, const char *name)
	{
        static ConVarRef tv_name("tv_name");
        static int counter = 0;
        SCOPED_INCREMENT(rc_CBaseServer_CreateFakeClient);
        SCOPED_INCREMENT_IF(rc_CBaseServer_CreateFakeClient_HLTV, tv_name.GetString() == name);
        if (tv_name.GetString() != name) {
            name = STRING(AllocPooledString(CFmtStr("TFBot %d", ++counter)));
        }

        //ForEachEntity([](CBaseEntity *ent){
         //   Msg("Entity %d %s\n", ent->entindex(), ent->GetClassname());
		//	});
		return DETOUR_MEMBER_CALL(CBaseServer_CreateFakeClient)(name);
	}

    DETOUR_DECL_MEMBER(void, CServerGameClients_ClientActive, edict_t *pEdict, bool bLoadGame)
	{
        
        Msg("EntityEnd %d\n", pEdict);
        if (pEdict != nullptr) {
            Msg("EntityEnd2 %d\n", pEdict->m_EdictIndex);
        }
        DETOUR_MEMBER_CALL(CServerGameClients_ClientActive)(pEdict, bLoadGame);
        Msg("EntityAfter\n");
        
    }

    DETOUR_DECL_STATIC(void, ClientActive, edict_t *pEdict, bool bLoadGame)
	{
        Msg("ClientActivePre\n");
        DETOUR_STATIC_CALL(ClientActive)(pEdict, bLoadGame);
        ForEachEntity([](CBaseEntity *ent){
            Msg("EntityPost %d %s\n", ent->entindex(), ent->GetClassname());
            ent->PostClientActive();
			});
        Msg("ClientActivePost\n");
    }

    DETOUR_DECL_MEMBER(CBaseClient *, CBaseServer_GetFreeClient, netadr_t &adr)
	{
        if (!ExtraSlotsEnabled() || gpGlobals->maxClients < 34) return DETOUR_MEMBER_CALL(CBaseServer_GetFreeClient)(adr);

        if (rc_CBaseServer_CreateFakeClient) {
            std::vector<CBaseClient *> clientList;
            auto server = reinterpret_cast<CBaseServer *>(this);

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
            
            int desiredSlot = 33;
            if (!rc_CBaseServer_CreateFakeClient_HLTV) {
                if (!sig_perf_hltv_allow_bots_extra_slot.GetBool()) {
                    desiredSlot = 32;
                }
                else {
			        static ConVarRef visible_max_players("sv_visiblemaxplayers");
			        static ConVarRef sig_mvm_robot_limit_override("sig_mvm_robot_limit_override");
			        static ConVarRef tv_enable("tv_enable");
                    desiredSlot = MIN(MAX(32,visible_max_players.GetInt() + sig_mvm_robot_limit_override.GetInt() - 1), server->GetMaxClients() - 1);
                }
            }
            
            for (int i = desiredSlot; i >= 0; i--) {
                CBaseClient *client = static_cast<CBaseClient *>(server->GetClient(i));
                static ConVarRef tv_enable("tv_enable");
                if (!client->IsConnected() 
                && !client->IsFakeClient() 
                && !(i == 33 && !rc_CBaseServer_CreateFakeClient_HLTV && tv_enable.GetBool())) {
                    return client;
                }
            }

            return nullptr;
        }
		auto client = DETOUR_MEMBER_CALL(CBaseServer_GetFreeClient)(adr);
        if ( !(sig_perf_hltv_allow_bots_extra_slot.GetBool() && rc_CBaseServer_CreateFakeClient) && !rc_CBaseServer_CreateFakeClient_HLTV && client->GetPlayerSlot() > 32) {
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
		    maxplayers = sig_perf_hltv_use_special_slot.GetInt();
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
        Msg("Element %d offset %d\n", iElement, playerListOffset);
        return DETOUR_STATIC_CALL(SendProxy_PlayerList)(pProp, pStruct, pData, pOut, iElement + playerListOffset, objectID);
    }
    DETOUR_DECL_STATIC(int, SendProxyArrayLength_PlayerArray, const void *pStruct, int objectID)
	{
		int count = DETOUR_STATIC_CALL(SendProxyArrayLength_PlayerArray)(pStruct, objectID);
        int countpre = count;
        if (ExtraSlotsEnabled()) {
            auto team = (CTeam *)(pStruct);

            for (int i = 0; i < countpre; i++) {
                if (ENTINDEX(team->GetPlayer(i)) > 33) {
                    count--;
                }
            }
        }
        Msg("Count %d Pre %d\n", count, countpre);
        return count;
    }

	DETOUR_DECL_MEMBER(void, CTeam_AddPlayer, CBasePlayer *player)
	{
        auto team = reinterpret_cast<CTFTeam *>(this);
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

	DETOUR_DECL_MEMBER(void, CTeam_RemovePlayer, CBasePlayer *player)
	{
        auto team = reinterpret_cast<CTFTeam *>(this);
        if (team->m_hLeader == player) {
            team->m_hLeader = nullptr;
        }
        DETOUR_MEMBER_CALL(CTFTeam_RemovePlayer)(player);
    }

	DETOUR_DECL_MEMBER(int, CGameMovement_GetPointContentsCached, const Vector &point, int slot)
	{
        auto movement = reinterpret_cast<CGameMovement *>(this);
        if (movement->player != nullptr && ENTINDEX(movement->player) > 33) {
            return enginetrace->GetPointContents(point);
        }
        return DETOUR_MEMBER_CALL(CGameMovement_GetPointContentsCached)(point, slot);
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

    DETOUR_DECL_MEMBER(void, CTFPlayerResource_UpdateConnectedPlayer, int index, CTFPlayer *player)
	{
        //Msg("Update connected player %d %d\n", index, ENTINDEX(player));
        int teampre = player->GetTeamNumber();
        player->SetTeamNumber(TEAM_SPECTATOR);
        DETOUR_MEMBER_CALL(CTFPlayerResource_UpdateConnectedPlayer)(index, player);
        player->SetTeamNumber(teampre);
        if (index > 0 && index < 34)
            TFPlayerResource()->m_iTeam.SetIndex(teampre, index);
    }

    DETOUR_DECL_MEMBER(void, CTFPlayerResource_UpdateDisconnectedPlayer, int index)
	{
        //Msg("Update disconnected player %d\n", index);

        TFPlayerResource()->m_iAccountID.SetIndex(0, index);
        DETOUR_MEMBER_CALL(CTFPlayerResource_UpdateDisconnectedPlayer)(index);
    }
    

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
    /*DETOUR_DECL_MEMBER(bool, IGameEventManager2_FireEvent, IGameEvent *event, bool bDontBroadcast)
	{
		auto mgr = reinterpret_cast<IGameEventManager2 *>(this);
		
		if (event != nullptr && sig_perf_hltv_allow_bots_extra_slot.GetBool() && strcmp(event->GetName(), "player_death") == 0) {
            auto victim = UTIL_PlayerByUserId(event->GetInt("userid"));
            auto attacker = UTIL_PlayerByUserId(event->GetInt("attacker"));
            auto assister = UTIL_PlayerByUserId(event->GetInt("assister"));
            auto player33 = UTIL_PlayerByIndex(33);
            if (player33 != nullptr) {
                if (ENTINDEX(victim) > 33) {
                    event->SetInt("userid", player33->GetUserID());
                }
            }
        }
    }*/
	DETOUR_DECL_MEMBER(void, CTFGameRules_GetTaggedConVarList, KeyValues *kv)
	{
        DETOUR_MEMBER_CALL(CTFGameRules_GetTaggedConVarList)(kv);
        FOR_EACH_SUBKEY(kv, subkey) {
            //Msg("CVAR %s TAG %s \n", subkey->GetString("convar"), subkey->GetString("tag"));
            if (FStrEq(subkey->GetString("convar"), "tf_pve_mode")) {
                kv->RemoveSubKey(subkey);
                subkey->deleteThis();
                break;
            }
        }
    }

    CON_COMMAND(sig_get_commands, "")
    {
        const ConCommandBase *cmd = icvar->GetCommands();
        for ( ; cmd; cmd = cmd->GetNext() )
        {
            Msg("%s\n", cmd->GetName());
        }
    }

	DETOUR_DECL_MEMBER(void, CBaseServer_RecalculateTags)
	{
        static ConCommandBase *bot_moveto = icvar->FindCommandBase("bot_moveto");
        static ConCommandBase *tf_stats_nogameplaycheck = icvar->FindCommandBase("tf_stats_nogameplaycheck");

        if (UTIL_PlayerByIndex(40) != nullptr) {
            Msg("Post %d\n", bot_moveto->GetNext());
            Msg("Post2 %d\n", tf_stats_nogameplaycheck->GetNext());
            const ConCommandBase *cmd = icvar->GetCommands();
            for ( ; cmd; cmd = cmd->GetNext() )
            {
                //Msg("%s\n", cmd->GetName());
            }
            return;
        }
        else {
            Msg("Pre %d\n", bot_moveto->GetNext());
            Msg("Pre2 %d\n", tf_stats_nogameplaycheck->GetNext());
        }
        DETOUR_MEMBER_CALL(CBaseServer_RecalculateTags)();
    }
    

	class CMod : public IMod, public IModCallbackListener
	{
	public:
		CMod() : IMod("Perf:HLTV_Optimize")
		{
            // Prevent the server from assuming its tv relay when sourcetv client is missing
            MOD_ADD_DETOUR_MEMBER(CHLTVServer_IsTVRelay,   "CHLTVServer::IsTVRelay");

			MOD_ADD_DETOUR_MEMBER(CHLTVServer_RunFrame,                      "CHLTVServer::RunFrame");
			MOD_ADD_DETOUR_MEMBER(CHLTVServer_UpdateTick,                      "CHLTVServer::UpdateTick");
			MOD_ADD_DETOUR_MEMBER(CTeamplayRoundBasedRules_State_Enter,        "CTeamplayRoundBasedRules::State_Enter");
			MOD_ADD_DETOUR_MEMBER(CHLTVServer_RestoreTick,                     "CHLTVServer::RestoreTick");
			MOD_ADD_DETOUR_MEMBER(CNetworkStringTable_UpdateMirrorTable,                     "CNetworkStringTable::UpdateMirrorTable");
            
			MOD_ADD_DETOUR_MEMBER(CNetworkStringTable_RestoreTick, "CNetworkStringTable::RestoreTick");
            
			MOD_ADD_DETOUR_MEMBER(NextBotPlayer_CTFPlayer_PhysicsSimulate,  "NextBotPlayer<CTFPlayer>::PhysicsSimulate");
			MOD_ADD_DETOUR_MEMBER(CBasePlayer_PhysicsSimulate,              "CBasePlayer::PhysicsSimulate");

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
			//MOD_ADD_DETOUR_MEMBER(CTFGameRules_GetTaggedConVarList, "CTFGameRules::GetTaggedConVarList");
			//MOD_ADD_DETOUR_MEMBER(CBaseServer_RecalculateTags, "CBaseServer::RecalculateTags");

            
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
            //MOD_ADD_DETOUR_MEMBER(CTFPlayerResource_UpdateConnectedPlayer, "CTFPlayerResource::UpdateConnectedPlayer");
            //MOD_ADD_DETOUR_MEMBER(CTFPlayerResource_UpdateDisconnectedPlayer, "CTFPlayerResource::UpdateDisconnectedPlayer");
            //MOD_ADD_DETOUR_MEMBER(CServerGameClients_ClientActive, "CServerGameClients::ClientActive");
            //MOD_ADD_DETOUR_STATIC(ClientActive, "ClientActive");
            MOD_ADD_DETOUR_MEMBER(CTFPlayerResource_SetPlayerClassWhenKilled, "CTFPlayerResource::SetPlayerClassWhenKilled");
            MOD_ADD_DETOUR_MEMBER(CBasePlayer_UpdatePlayerSound, "CBasePlayer::UpdatePlayerSound");
            MOD_ADD_DETOUR_MEMBER(CSoundEnt_Initialize, "CSoundEnt::Initialize");
            MOD_ADD_DETOUR_MEMBER(CVoiceGameMgr_ClientConnected, "CVoiceGameMgr::ClientConnected");
            
            
            
			//MOD_ADD_DETOUR_MEMBER(CTFPlayer_ShouldTransmit,               "CTFPlayer::ShouldTransmit");
            //MOD_ADD_DETOUR_STATIC(SendTable_CalcDelta,   "SendTable_CalcDelta");
		}

        virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

        virtual void OnEnablePost() override
		{

        }

        virtual void LevelInitPreEntity() override
		{
            if (sig_perf_hltv_use_special_slot.GetInt() > 33 && (gpGlobals->maxClients >= 32 && gpGlobals->maxClients < sig_perf_hltv_use_special_slot.GetInt())) {
                engine->ChangeLevel(STRING(gpGlobals->mapname), nullptr);
            }
            last_restore_tick = -1;
        }

        virtual void LevelShutdownPostEntity() override
		{

            if (sig_perf_hltv_use_special_slot.GetInt() > 33 && (gpGlobals->maxClients >= 32 && gpGlobals->maxClients < sig_perf_hltv_use_special_slot.GetInt())) {
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
                ft_SetupMaxPlayers(sig_perf_hltv_use_special_slot.GetInt());
            }
        }
	};
	CMod s_Mod;
    
	ConVar cvar_enable("sig_perf_hltv_optimize", "0", FCVAR_NOTIFY,
		"Mod: improve HLTV performance",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
