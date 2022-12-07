#include "mod.h"
#include "stub/tfbot.h"
#include "util/scope.h"
#include "util/clientmsg.h"
#include "util/misc.h"
#include "stub/tfplayer.h"
#include "stub/gamerules.h"
#include "stub/misc.h"
#include "stub/server.h"
#include "stub/team.h"
#include "stub/tfweaponbase.h"
#include "stub/tf_player_resource.h"
#include "util/iterate.h"
#include <gamemovement.h>

namespace Mod::Perf::HLTV_Optimize
{

    ConVar cvar_rate_between_rounds("sig_perf_hltv_rate_between_rounds", "8", FCVAR_NOTIFY,
		"Source TV snapshotrate between rounds");

    bool hltvServerEmpty = false;
    bool recording = false;

    std::unordered_multimap<CNetworkStringTable *, int> tickChanges;
    DETOUR_DECL_MEMBER(bool, CHLTVServer_IsTVRelay)
	{
        return false;
    }

    float old_snapshotrate = 16.0f;
    RefCount rc_CHLTVServer_RunFrame;
	DETOUR_DECL_MEMBER(void, CHLTVServer_RunFrame)
	{
        static bool limitSnapshotRate = false;
        static ConVarRef snapshotrate("tv_snapshotrate");
        SCOPED_INCREMENT(rc_CHLTVServer_RunFrame);
        CHLTVServer *hltvserver = reinterpret_cast<CHLTVServer *>(this);
        
        static ConVarRef tv_enable("tv_enable");
        static ConVarRef tv_maxclients("tv_maxclients");
        bool tv_enabled = tv_enable.GetBool();

        hltvServerEmpty = hltvserver->GetNumClients() == 0;
        auto state = TFGameRules()->State_Get();

        bool hasplayer = tv_maxclients.GetInt() != 0;
        IClient * hltvclient = nullptr;

        int clientCount = sv->GetClientCount();
        int hltvClientIndex;
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
                    hltvClientIndex = i;
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
            //Msg("Frames to keepa %d\n", hltvserver->CountClientFrames());
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

        if (!limitSnapshotRate && state != GR_STATE_RND_RUNNING && !hltvServerEmpty) {
            limitSnapshotRate = true;
            old_snapshotrate = snapshotrate.GetFloat();
            snapshotrate.SetValue(cvar_rate_between_rounds.GetFloat());
        }
        else if (limitSnapshotRate && (state == GR_STATE_RND_RUNNING || !hltvServerEmpty)) {
            limitSnapshotRate = false;
            snapshotrate.SetValue(old_snapshotrate);
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
            int framesToKeep = 2 + delay.GetFloat() / gpGlobals->interval_per_tick;
            if (delay.GetFloat() <= 0) {
                int framec = hltvserver->CountClientFrames() - 2;
                for (int i = 0; i < framec; i++)
                    hltvserver->RemoveOldestFrame();
            }
            //DevMsg("SendNow %d\n", gpGlobals->tickcount % tickcount == 0/*reinterpret_cast<CGameClient *>(hltvclient)->ShouldSendMessages()*/);
            // if (gpGlobals->tickcount % tickcount != 0)
            //    return;
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
        
        //TIME_SCOPE2(RESTORE);
        DETOUR_MEMBER_CALL(CHLTVServer_RestoreTick)(time);
        last_restore_tick = time;
    }

    //Do not delay stringtables in demo recording

    DETOUR_DECL_MEMBER(void, CNetworkStringTable_RestoreTick, int tick)
	{
        auto table = reinterpret_cast<CNetworkStringTable *>(this);
        if (tickChanges.size() == 0 && tick != 0) return;

        auto [first, last] = tickChanges.equal_range(table);
        bool hasChanges = tick == 0;

        for (auto it = first; it != last;) {
            int tickChange = it->second;
            if (tickChange <= tick) {
                hasChanges = true;
                it = tickChanges.erase(it);
            }
            else {
                it++;
            }
        }
        if (hasChanges/*tick == 0 || (last_change > last_restore_tick && last_change <= tick)*/) {
            DETOUR_MEMBER_CALL(CNetworkStringTable_RestoreTick)(tick);
        }
    }
    
    DETOUR_DECL_MEMBER(void, CNetworkStringTable_UpdateMirrorTable, int tick)
    {
        DETOUR_MEMBER_CALL(CNetworkStringTable_UpdateMirrorTable)(tick);
        auto table = reinterpret_cast<CNetworkStringTable *>(this);

        if (table->m_pMirrorTable != nullptr) {
            tickChanges.emplace(table->m_pMirrorTable, table->m_pMirrorTable->m_nTickCount);
        }
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


	class CMod : public IMod, public IModCallbackListener
	{
	public:
		CMod() : IMod("Perf:HLTV_Optimize")
		{
            // Prevent the server from assuming its tv relay when sourcetv client is missing
            MOD_ADD_DETOUR_MEMBER(CHLTVServer_IsTVRelay,   "CHLTVServer::IsTVRelay");

			MOD_ADD_DETOUR_MEMBER(CHLTVServer_RunFrame,                      "CHLTVServer::RunFrame");

			MOD_ADD_DETOUR_MEMBER(CHLTVServer_UpdateTick,                      "CHLTVServer::UpdateTick");
			MOD_ADD_DETOUR_MEMBER(CHLTVServer_RestoreTick,                     "CHLTVServer::RestoreTick");
			MOD_ADD_DETOUR_MEMBER(CNetworkStringTable_RestoreTick, "CNetworkStringTable::RestoreTick");
			MOD_ADD_DETOUR_MEMBER(CNetworkStringTable_UpdateMirrorTable,                     "CNetworkStringTable::UpdateMirrorTable");

			MOD_ADD_DETOUR_MEMBER(NextBotPlayer_CTFPlayer_PhysicsSimulate,  "NextBotPlayer<CTFPlayer>::PhysicsSimulate");
			MOD_ADD_DETOUR_MEMBER(CBasePlayer_PhysicsSimulate,              "CBasePlayer::PhysicsSimulate");
                        
			//MOD_ADD_DETOUR_MEMBER(CTFPlayer_ShouldTransmit,               "CTFPlayer::ShouldTransmit");
            //MOD_ADD_DETOUR_STATIC(SendTable_CalcDelta,   "SendTable_CalcDelta");
		}

        virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

        virtual void LevelInitPreEntity() override
		{
            tickChanges.clear();
            last_restore_tick = -1;
        }
	};
	CMod s_Mod;
    
	ConVar cvar_enable("sig_perf_hltv_optimize", "0", FCVAR_NOTIFY,
		"Mod: improve HLTV performance",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
