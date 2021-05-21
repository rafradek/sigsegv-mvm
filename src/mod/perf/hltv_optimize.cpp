#include "mod.h"
#include "util/scope.h"
#include "util/clientmsg.h"
#include "stub/tfplayer.h"
#include "stub/gamerules.h"
#include "stub/misc.h"
#include "stub/server.h"
#include "stub/tfweaponbase.h"

class CNetworkStringTable  : public INetworkStringTable
{
public:
	virtual			~CNetworkStringTable( void );
	virtual void	Dump( void );
	virtual void	Lock( bool bLock );

	TABLEID					m_id;
	char					*m_pszTableName;
	// Must be a power of 2, so encoding can determine # of bits to use based on log2
	int						m_nMaxEntries;
	int						m_nEntryBits;
	int						m_nTickCount;
	int						m_nLastChangedTick;
	bool					m_bChangeHistoryEnabled : 1;
	bool					m_bLocked : 1;
	bool					m_bAllowClientSideAddString : 1;
	bool					m_bUserDataFixedSize : 1;
	bool					m_bIsFilenames : 1;
	int						m_nUserDataSize;
	int						m_nUserDataSizeBits;
	pfnStringChanged		m_changeFunc;
	void					*m_pObject;
	CNetworkStringTable		*m_pMirrorTable;
};

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

        if (hasplayer && hltvclient == nullptr) {
            DevMsg("spawning sourcetv\n");
            static ConVarRef tv_name("tv_name");
            IClient *client = reinterpret_cast<CBaseServer *>(sv)->CreateFakeClient(tv_name.GetString());
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
            int tickcount = 32.0f / (snapshotrate.GetFloat() * gpGlobals->interval_per_tick);
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
			//MOD_ADD_DETOUR_MEMBER(CTFPlayer_ShouldTransmit,               "CTFPlayer::ShouldTransmit");
            //MOD_ADD_DETOUR_STATIC(SendTable_CalcDelta,   "SendTable_CalcDelta");
		}

        virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

        virtual void LevelInitPreEntity() override
		{
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
