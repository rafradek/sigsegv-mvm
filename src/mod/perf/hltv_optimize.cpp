#include "mod.h"
#include "util/scope.h"
#include "stub/tfplayer.h"
#include "stub/gamerules.h"
#include "stub/misc.h"


class CHLTVServer
{
public:
    void StartMaster(IClient *client)                      {        ft_StartMaster(this, client); }
    
private:
    static MemberFuncThunk<CHLTVServer *, void, IClient *>              ft_StartMaster;
};

class CBaseServer
{
public:
    IClient *CreateFakeClient(const char *name)                      { return ft_CreateFakeClient(this, name); }
    
private:
    static MemberFuncThunk<CBaseServer *, IClient *, const char *>              ft_CreateFakeClient;
};

class CGameClient 
{
public:
    bool ShouldSendMessages()                      { return ft_ShouldSendMessages(this); }
    
private:
    static MemberFuncThunk<CGameClient *, bool>              ft_ShouldSendMessages;
};


MemberFuncThunk<CHLTVServer *, void, IClient *> CHLTVServer::ft_StartMaster("CHLTVServer::StartMaster");
MemberFuncThunk<CBaseServer *, IClient *, const char *> CBaseServer::ft_CreateFakeClient("CBaseServer::CreateFakeClient");
MemberFuncThunk<CGameClient *, bool>              CGameClient::ft_ShouldSendMessages("CGameClient::ShouldSendMessages");

namespace Mod::Perf::HLTV_Optimize
{
    ConVar cvar_rate_between_rounds("sig_perf_hltv_rate_between_rounds", "8", FCVAR_NOTIFY,
		"Source TV snapshotrate between rounds");

    DETOUR_DECL_MEMBER(bool, CHLTVServer_IsTVRelay)
	{
        return false;
    }

    float old_snapshotrate = 16.0f;
    RefCount rc_CHLTVServer_RunFrame;
	DETOUR_DECL_MEMBER(void, CHLTVServer_RunFrame)
	{
        static ConVarRef tv_enable("tv_enable");
        bool tv_enabled = tv_enable.GetBool();

        bool hasplayer = false;
        IClient * hltvclient = nullptr;


        for ( int i=0 ; i< sv->GetClientCount() ; i++ )
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
                if ((hasplayer || !tv_enabled) && hltvclient != nullptr)
                    break;
            }
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
            static ConVarRef tv_maxclients("tv_maxclients");
            if (tv_maxclients.GetInt() == 0) {
                static ConVarRef snapshotrate("tv_snapshotrate");
                int tickcount = 4.0f / (snapshotrate.GetFloat() * gpGlobals->interval_per_tick);
                SCOPED_INCREMENT(rc_CHLTVServer_RunFrame);
                //DevMsg("SendNow %d\n", gpGlobals->tickcount % tickcount == 0/*reinterpret_cast<CGameClient *>(hltvclient)->ShouldSendMessages()*/);
                if (gpGlobals->tickcount % tickcount != 0)
                    return;
            }
        }
		DETOUR_MEMBER_CALL(CHLTVServer_RunFrame)();
	}

	DETOUR_DECL_MEMBER(void, CTeamplayRoundBasedRules_State_Enter, gamerules_roundstate_t newState)
	{
        static ConVarRef tv_maxclients("tv_maxclients");
        if (newState != GR_STATE_RND_RUNNING && tv_maxclients.GetInt() == 0) {
            static ConVarRef snapshotrate("tv_snapshotrate");
            if (snapshotrate.GetFloat() != cvar_rate_between_rounds.GetFloat()) {
                old_snapshotrate = snapshotrate.GetFloat();
                snapshotrate.SetValue(cvar_rate_between_rounds.GetFloat());
            }
        }
        else
        {
            static ConVarRef snapshotrate("tv_snapshotrate");
            if (snapshotrate.GetFloat() == cvar_rate_between_rounds.GetFloat()) {
                snapshotrate.SetValue(old_snapshotrate);
            }
        }
        DETOUR_MEMBER_CALL(CTeamplayRoundBasedRules_State_Enter)(newState);
    }

    RefCount rc_CServerGameEnts_CheckTransmit;
    DETOUR_DECL_MEMBER(int, CTFPlayer_ShouldTransmit, CCheckTransmitInfo *info)
	{
        auto result = DETOUR_MEMBER_CALL(CTFPlayer_ShouldTransmit)(info);
        //DevMsg("from %d", rc_CServerGameEnts_CheckTransmit);
        DevMsg("transmitted %d\n", info->m_pTransmitEdict->Get( reinterpret_cast<CBaseEntity *>(this)->entindex()));
        if (result != FL_EDICT_DONTSEND) {
            if (gpGlobals->tickcount % 66 == 0)
                return result;

            CBaseEntity *ent = GetContainingEntity(info->m_pClientEnt);

            if (ent != nullptr && ent != reinterpret_cast<CBaseEntity *>(this) && ent->IsPlayer()) {
                //DevMsg("Setting dont send \n");
                return FL_EDICT_DONTSEND;
            }
        }
		return result;
	}
    
    std::unordered_set<CBaseEntity *> just_spawned_set;

    DETOUR_DECL_MEMBER(void, CTFGameRules_OnPlayerSpawned, CTFPlayer *player)
	{
        just_spawned_set.insert(player);
		DETOUR_MEMBER_CALL(CTFGameRules_OnPlayerSpawned)(player);
    }
    
    int last_tick_transmit;
    ConVar cvar_frame_update_skip("sig_perf_frame_update_skip", "1", FCVAR_NOTIFY,
		"Mod: frame skipping for bots");

    DETOUR_DECL_MEMBER(void, CServerGameEnts_CheckTransmit, CCheckTransmitInfo *info, unsigned short *indices, int edictsnum)
	{
        DETOUR_MEMBER_CALL(CServerGameEnts_CheckTransmit)(info, indices, edictsnum);
        if (last_tick_transmit != gpGlobals->tickcount) {
            for (int i = 1; i < edictsnum; i++) {
                int iEdict = indices[i];

                if (iEdict <= gpGlobals->maxClients) {
                    edict_t *edict = INDEXENT(iEdict);
                    //DevMsg( "notful ");
                    if (edict->m_fStateFlags & (FL_EDICT_CHANGED | FL_FULL_EDICT_CHANGED)) {

                        CBasePlayer *ent = reinterpret_cast<CBasePlayer *>(GetContainingEntity(edict));
                        bool isalive = ent->IsAlive();
                        if (ent->GetFlags() & FL_FAKECLIENT && ((!isalive && ent->GetDeathTime() + 1.0f < gpGlobals->curtime)
                            || (isalive && gpGlobals->tickcount % cvar_frame_update_skip.GetInt() != 0 && just_spawned_set.find(ent) == just_spawned_set.end()))) {
                            edict->m_fStateFlags &= ~(FL_EDICT_CHANGED | FL_FULL_EDICT_CHANGED);
                        }
                    }
                
                }
                else if (iEdict > gpGlobals->maxClients) {
                    break;
                }
            }
            last_tick_transmit = gpGlobals->tickcount;
            just_spawned_set.clear();
        }
	}

    /*int count_tick;
    int last_tick;
    DETOUR_DECL_STATIC(void, SV_PackEntity, int edictnum, edict_t *edict, ServerClass *serverclass, void *snapshot)
	{
        if (last_tick != gpGlobals->tickcount) {
            DevMsg("pack calls: %d\n", count_tick);
            count_tick = 0;
            last_tick = gpGlobals->tickcount;
        }
        count_tick ++;
        DETOUR_STATIC_CALL(SV_PackEntity)(edictnum, edict, serverclass, snapshot);
    }*/

    DETOUR_DECL_STATIC(int, SendTable_CalcDelta, const SendTable *pTable,
	
	const void *pFromState,
	const int nFromBits,

	const void *pToState,
	const int nToBits,

	int *pDeltaProps,
	int nMaxDeltaProps,

	const int objectID)
	{
        int result = DETOUR_STATIC_CALL(SendTable_CalcDelta)(pTable, pFromState, nFromBits, pToState, nToBits, pDeltaProps, nMaxDeltaProps, objectID);
        if (result != 0 && objectID > 0 && objectID <= gpGlobals->maxClients) {
            edict_t *edict = INDEXENT(objectID);
            if (!(edict->m_fStateFlags & FL_EDICT_CHANGED)) {
                edict->m_fStateFlags |= FL_EDICT_CHANGED;
            }
        }
        return result;
	}

	DETOUR_DECL_MEMBER(void, NextBotPlayer_CTFPlayer_PhysicsSimulate)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (player->GetTeamNumber() <= TEAM_SPECTATOR && !player->IsAlive() && gpGlobals->curtime - player->GetDeathTime() > 1.0f && gpGlobals->tickcount % 64 != 0)
			return;
            
		DETOUR_MEMBER_CALL(NextBotPlayer_CTFPlayer_PhysicsSimulate)();
	}

	class CMod : public IMod
	{
	public:
		CMod() : IMod("Perf:HLTV_Optimize")
		{
            // Prevent the server from assuming its tv relay when sourcetv client is missing
            MOD_ADD_DETOUR_MEMBER(CHLTVServer_IsTVRelay,   "CHLTVServer::IsTVRelay");

			MOD_ADD_DETOUR_MEMBER(CHLTVServer_RunFrame,                     "CHLTVServer::RunFrame");
			MOD_ADD_DETOUR_MEMBER(CTeamplayRoundBasedRules_State_Enter,     "CTeamplayRoundBasedRules::State_Enter");
			//MOD_ADD_DETOUR_MEMBER(CTFPlayer_ShouldTransmit,               "CTFPlayer::ShouldTransmit");

			MOD_ADD_DETOUR_MEMBER(CServerGameEnts_CheckTransmit,               "CServerGameEnts::CheckTransmit");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_OnPlayerSpawned,               "CTFGameRules::OnPlayerSpawned");

            MOD_ADD_DETOUR_STATIC(SendTable_CalcDelta,   "SendTable_CalcDelta");
			MOD_ADD_DETOUR_MEMBER(NextBotPlayer_CTFPlayer_PhysicsSimulate, "NextBotPlayer<CTFPlayer>::PhysicsSimulate");
            //MOD_ADD_DETOUR_STATIC(SV_PackEntity,   "SV_PackEntity");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_perf_hltv_optimize", "0", FCVAR_NOTIFY,
		"Mod: improve HLTV performance",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
