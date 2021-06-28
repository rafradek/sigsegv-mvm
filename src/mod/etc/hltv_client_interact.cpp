#include "mod.h"
#include "stub/misc.h"
#include "stub/server.h"
#include "util/clientmsg.h"
#include "util/misc.h"

namespace Mod::Etc::HLTV_Client_Interact
{
    DETOUR_DECL_MEMBER(bool, CHLTVClient_ExecuteStringCommand, const char *cmd)
	{
        auto client = reinterpret_cast<CHLTVClient *>(this);
        CCommand args;
        if (!args.Tokenize(cmd)) {
            return DETOUR_MEMBER_CALL(CHLTVClient_ExecuteStringCommand)(cmd);
        }

        if (FStrEq(args[0], "say")) {
            PrintToChatAll(CFmtStr("%s : %s", client->GetClientName(), args[1]));
            return true;
        }
        else if (FStrEq(args[0], "spectators")) {
            client->ClientPrintf("Spectators:\n");
            for (int i = 0; i < hltv->GetClientCount(); i++) {
                IClient *cl = hltv->GetClient(i);
                client->ClientPrintf("%s\n", cl->GetClientName());
            }

            return true;
        }

		return DETOUR_MEMBER_CALL(CHLTVClient_ExecuteStringCommand)(cmd);
	}

    DETOUR_DECL_MEMBER(IClient *, CHLTVServer_ConnectClient, netadr_t &adr, int protocol, int challenge, int clientChallenge, int authProtocol, 
									 const char *name, const char *password, const char *hashedCDkey, int cdKeyLen)
	{
        auto client = DETOUR_MEMBER_CALL(CHLTVServer_ConnectClient)(adr, protocol, challenge, clientChallenge, authProtocol, name, password, hashedCDkey, cdKeyLen);
        
        if (client != nullptr) {
            PrintToChatAll(CFmtStr("Source TV spectator %s joined", client->GetClientName()));
        }
        return client;
    }
    
	DETOUR_DECL_MEMBER(bool, CTFPlayer_ClientCommand, const CCommand& args)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (player != nullptr) {
			if (strcmp(args[0], "sig_tvspectators") == 0) {
                ClientMsg(player, "Spectators:\n");
                for (int i = 0; i < hltv->GetClientCount(); i++) {
                    IClient *cl = hltv->GetClient(i);
                    ClientMsg(player, "%s\n", cl->GetClientName());
                }
                return true;
            }
		}
		
		return DETOUR_MEMBER_CALL(CTFPlayer_ClientCommand)(args);
	}


	class CMod : public IMod
	{
	public:
		CMod() : IMod("Perf:HLTV_Client_Interact")
		{
			MOD_ADD_DETOUR_MEMBER(CHLTVClient_ExecuteStringCommand,              "CHLTVClient::ExecuteStringCommand");
			MOD_ADD_DETOUR_MEMBER(CHLTVServer_ConnectClient,                     "CHLTVServer::ConnectClient");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ClientCommand,                       "CTFPlayer::ClientCommand");
            
			//MOD_ADD_DETOUR_MEMBER(CTFPlayer_ShouldTransmit,               "CTFPlayer::ShouldTransmit");
            //MOD_ADD_DETOUR_STATIC(SendTable_CalcDelta,   "SendTable_CalcDelta");
		}
	};
	CMod s_Mod;
    
	ConVar cvar_enable("sig_etc_hltv_client_interact", "0", FCVAR_NOTIFY,
		"Mod: allow HLTV clients to interact with server players",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}