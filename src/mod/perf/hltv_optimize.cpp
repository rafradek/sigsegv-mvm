#include "mod.h"
#include "util/scope.h"
#include "util/clientmsg.h"
#include "util/misc.h"
#include "stub/tfplayer.h"
#include "stub/gamerules.h"
#include "stub/misc.h"
#include "stub/server.h"
#include "stub/team.h"
#include "stub/tfweaponbase.h"

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
        SCOPED_INCREMENT(rc_CBaseServer_CreateFakeClient);
        SCOPED_INCREMENT_IF(rc_CBaseServer_CreateFakeClient_HLTV, tv_name.GetString() == name);
		return DETOUR_MEMBER_CALL(CBaseServer_CreateFakeClient)(name);
	}
    
    DETOUR_DECL_MEMBER(CBaseClient *, CBaseServer_GetFreeClient, netadr_t &adr)
	{
        if (!ExtraSlotsEnabled()) return DETOUR_MEMBER_CALL(CBaseServer_GetFreeClient)(adr);

        if (rc_CBaseServer_CreateFakeClient_HLTV) {
            std::vector<CBaseClient *> clientList;

            auto server = reinterpret_cast<CBaseServer *>(this);
            for (int i = 0; i < server->GetClientCount(); i++) {
                CBaseClient *client = static_cast<CBaseClient *>(server->GetClient(i));
                if (!client->IsConnected() && !client->IsFakeClient()) {
                    clientList.push_back(client);
                    client->m_bFakePlayer = true;
                }
            }

            CBaseClient *lastClient = nullptr;
            while (server->GetClientCount() != server->GetMaxClients()) {
                lastClient = DETOUR_MEMBER_CALL(CBaseServer_GetFreeClient)(adr);
                if (lastClient != nullptr) {
                    clientList.push_back(lastClient);
                    lastClient->m_bFakePlayer = true;
                }
            }
            
            for (CBaseClient *client : clientList) {
                client->m_bFakePlayer = false;
            }

            return lastClient;
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
        return DETOUR_STATIC_CALL(SendProxy_PlayerList)(pProp, pStruct, pData, pOut, iElement + playerListOffset, objectID);
    }
    DETOUR_DECL_STATIC(int, SendProxyArrayLength_PlayerArray, const void *pStruct, int objectID)
	{
		int count = DETOUR_STATIC_CALL(SendProxyArrayLength_PlayerArray)(pStruct, objectID);
        if (ExtraSlotsEnabled()) {
            auto team = (CTeam *)(pStruct);

            for (int i = 0; i < count; i++) {
                if (ENTINDEX(team->GetPlayer(i)) > 33) {
                    count--;
                }
            }
        }
        return count;
    }

	DETOUR_DECL_MEMBER(void, CTFTeam_AddPlayer, CBasePlayer *player)
	{
        DETOUR_MEMBER_CALL(CTFTeam_AddPlayer)(player);
        auto team = reinterpret_cast<CTFTeam *>(this);
        if (ENTINDEX(player) > 33 && team->m_hLeader == player) {
            team->m_hLeader = nullptr;
        }
    }

	DETOUR_DECL_MEMBER(void, CTeam_RemovePlayer, CBasePlayer *player)
	{
        auto team = reinterpret_cast<CTFTeam *>(this);
        if (team->m_hLeader == player) {
            team->m_hLeader = nullptr;
        }
        DETOUR_MEMBER_CALL(CTFTeam_RemovePlayer)(player);
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
			//MOD_ADD_DETOUR_STATIC(SendProxyArrayLength_PlayerArray,    "SendProxyArrayLength_PlayerArray");
			//MOD_ADD_DETOUR_MEMBER(CTFTeam_AddPlayer, "CTFTeam::AddPlayer");
			MOD_ADD_DETOUR_MEMBER(CTeam_RemovePlayer, "CTeam::RemovePlayer");
            
            
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
