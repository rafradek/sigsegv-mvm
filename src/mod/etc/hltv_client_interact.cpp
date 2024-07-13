#include "mod.h"
#include "stub/misc.h"
#include "stub/server.h"
#include "util/clientmsg.h"
#include "util/misc.h"
#include "mod/common/commands.h"

namespace Mod::Etc::HLTV_Client_Interact
{
    DETOUR_DECL_MEMBER(bool, CHLTVClient_ExecuteStringCommand, const char *cmd)
	{
        auto client = reinterpret_cast<CHLTVClient *>(this);
        CCommand args;
        if (!args.Tokenize(cmd)) {
            return DETOUR_MEMBER_CALL(cmd);
        }

        if (FStrEq(args[0], "say")) {
            PrintToChatAll(CFmtStr("(Source TV) %s : %s", client->GetClientName(), args[1]));
            return true;
        }
        else if (FStrEq(args[0], "spectators")) {
            std::string str ("Spectators:\n");
            for (int i = 0; i < hltv->GetClientCount(); i++) {
                IClient *cl = hltv->GetClient(i);
                str += cl->GetClientName();
                str += '\n';
            }
            client->ClientPrintf(str.c_str());
            IGameEvent *event = gameeventmanager->CreateEvent("hltv_chat");
            if (event == nullptr) return false;
            event->SetString("text", str.c_str());
            client->FireGameEvent(event);
            gameeventmanager->FreeEvent(event);

            return true;
        }

		return DETOUR_MEMBER_CALL(cmd);
	}

    ConVar sig_etc_hltv_notify_clients("sig_etc_hltv_notify_clients", "1", FCVAR_NOTIFY,
		"Mod: Notify players about connecting Source TV spectators");

    class TimerConnect : public ITimedEvent {
        virtual ResultType OnTimer(ITimer *pTimer, void *pData) {
            
            IGameEvent *event = gameeventmanager->CreateEvent("hltv_chat");
            if (event == nullptr) return Pl_Stop;
            event->SetString("text", "You can chat with players on the map\n");
            auto client = reinterpret_cast<CHLTVClient *>(pData);
            client->FireGameEvent(event);
            gameeventmanager->FreeEvent(event);
            return Pl_Stop;
        }
        virtual void OnTimerEnd(ITimer *pTimer, void *pData) {}
    } timer_connect;

    DETOUR_DECL_MEMBER(IClient *, CHLTVServer_ConnectClient, netadr_t &adr, int protocol, int challenge, int clientChallenge, int authProtocol, 
									 const char *name, const char *password, const char *hashedCDkey, int cdKeyLen)
	{
        auto client = DETOUR_MEMBER_CALL(adr, protocol, challenge, clientChallenge, authProtocol, name, password, hashedCDkey, cdKeyLen);
        
        return client;
    }

    VHOOK_DECL(void, CHLTVClient_Disconnect, char const *reason)
	{
        auto client = reinterpret_cast<CHLTVClient *>(this);
        if (sig_etc_hltv_notify_clients.GetBool() && client->m_nSignonState >= 5) {
            PrintToChatAllSM(2, "%t\n", "Source TV spectator disconnected", client->GetClientName());
        }
        VHOOK_CALL(reason);
    }


    VHOOK_DECL(void, CBaseClient_ActivatePlayer)
	{
        auto client = reinterpret_cast<CHLTVClient *>(this);
        VHOOK_CALL();
        timersys->CreateTimer(&timer_connect, 3.0f, client, 0);
        if (sig_etc_hltv_notify_clients.GetBool()) {
            PrintToChatAllSM(2, "%t\n", "Source TV spectator joined", client->GetClientName());
        }

    }

	class CMod : public IMod
	{
	public:
		CMod() : IMod("Perf:HLTV_Client_Interact")
		{
			MOD_ADD_DETOUR_MEMBER(CHLTVClient_ExecuteStringCommand,              "CHLTVClient::ExecuteStringCommand");
			MOD_ADD_DETOUR_MEMBER(CHLTVServer_ConnectClient,                     "CHLTVServer::ConnectClient");
            MOD_ADD_VHOOK(CHLTVClient_Disconnect, TypeName<CHLTVClient>(),       "CBaseClient::Disconnect");
            MOD_ADD_VHOOK(CBaseClient_ActivatePlayer, TypeName<CHLTVClient>(),   "CBaseClient::ActivatePlayer");
			//MOD_ADD_DETOUR_MEMBER(CTFPlayer_ShouldTransmit,               "CTFPlayer::ShouldTransmit");
            //MOD_ADD_DETOUR_STATIC(SendTable_CalcDelta,   "SendTable_CalcDelta");
		}
	};
	CMod s_Mod;
    
    ModCommand sig_tvspectators("sig_tvspectators", [](CCommandPlayer *player, const CCommand& args){
		std::string str(FormatTextForPlayerSM(player, 1, "%t\n", "sig_tvspectators"));
        for (int i = 0; i < hltv->GetClientCount(); i++) {
            IClient *cl = hltv->GetClient(i);
            str += cl->GetClientName();
            str += '\n';
        }
        ModCommandResponse("%s", str.c_str());
	}, &s_Mod);

	ConVar cvar_enable("sig_etc_hltv_client_interact", "0", FCVAR_NOTIFY,
		"Mod: allow HLTV clients to interact with server players",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}