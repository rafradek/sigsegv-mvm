#include "mod.h"
#include "stub/baseentity.h"
#include "stub/tfweaponbase.h"
#include "stub/projectiles.h"
#include "stub/objects.h"
#include "stub/tfentities.h"
#include "stub/nextbot_cc.h"
#include "stub/gamerules.h"
#include "stub/nav.h"
#include "stub/misc.h"
#include "link/link.h"
#include "util/backtrace.h"
#include "util/clientmsg.h"
#include "util/misc.h"
#include "util/iterate.h"
#include "util/netmessages.h"
#include "tier1/CommandBuffer.h"
#include <steam/steam_gameserver.h>
#include <steam/isteamgameserver.h>

namespace Mod::Etc::Server_Redirects
{
	int currentFakeIp;
	int currentPublicIp;
	

	bool hasRedirect = false;

	GlobalThunk<bool> g_bUseFakeIP("g_bUseFakeIP");
	GlobalThunk<int> g_nFakeIP("g_nFakeIP");
	GlobalThunk<short[2]> g_arFakePorts("g_arFakePorts");
	GlobalThunk<CSteamGameServerAPIContext *> steamgameserverapicontext("steamgameserverapicontext");

	ConVar redirect_only("sig_etc_server_redirects_redirect_only", "0", FCVAR_NOTIFY,
		"This server is for redirects only",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			if (static_cast<ConVar *>(pConVar)->GetBool()) {
				static ConVarRef sv_password("sv_password");
				static ConVarRef tf_mm_strict("tf_mm_strict");
				sv_password.SetValue("dest");
				tf_mm_strict.SetValue("2");
			}
		});

	bool clientWasRedirected[ABSOLUTE_PLAYER_LIMIT];

	DETOUR_DECL_MEMBER(IClient *, CBaseServer_ConnectClient, netadr_t &adr, int protocol, int challenge, int clientChallenge, int authProtocol, 
									 const char *name, const char *password, const char *hashedCDkey, int cdKeyLen)
	{
		hasRedirect = false;
        auto client = DETOUR_MEMBER_CALL(adr, protocol, challenge, clientChallenge, authProtocol, name, password, hashedCDkey, cdKeyLen);
		if (client != nullptr) {
			clientWasRedirected[client->GetPlayerSlot()] = hasRedirect;
		}
        if (hasRedirect && redirect_only.GetBool() && client != nullptr && client->GetNetChannel() != nullptr) {

			static ConVarRef hostport("hostport");
			SteamIPAddress_t address;
			short port = hostport.GetInt();
			if (g_bUseFakeIP) {
				address.m_unIPv4 = g_nFakeIP.GetRef();
				port = g_arFakePorts.GetRef()[0];
			}
			else if (steamgameserverapicontext != nullptr && steamgameserverapicontext->SteamGameServer() != nullptr) {
				address = steamgameserverapicontext->SteamGameServer()->GetPublicIP();
			}

			CmdMessage message;
			message.dest = password;
			size_t start = message.dest.find_first_of('=')+1;
			size_t end = message.dest.find_first_of('&');
			message.dest = message.dest.substr(start, end-start);
			if (message.dest == std::format("{}.{}.{}.{}:{}", (address.m_unIPv4 >> 24) & 255, (address.m_unIPv4 >> 16) & 255, (address.m_unIPv4 >> 8) & 255, address.m_unIPv4 & 255, port)) {
				clientWasRedirected[client->GetPlayerSlot()] = false;
			}
			else {
				message.dest = "redirect "s + message.dest;
				client->SendNetMsg(message, true);
			}
		}
        return client;
    }
	
	DETOUR_DECL_MEMBER(void, CServerGameClients_ClientPutInServer, edict_t *edict, const char *playername)
	{
		if (redirect_only.GetBool() && !sv->GetClient(edict->m_EdictIndex-1)->IsFakeClient()) {
			sv->GetClient(edict->m_EdictIndex-1)->Disconnect("failed to redirect");
			return;
		}
        DETOUR_MEMBER_CALL(edict, playername);
    }
	
	DETOUR_DECL_MEMBER(const char *, CServerGameDLL_GetServerBrowserMapOverride)
	{
		if (redirect_only.GetBool()) {
			return "redirecting (join game to connect to proper server)";
		}
        return DETOUR_MEMBER_CALL();
    }

	

	DETOUR_DECL_MEMBER(bool, CBaseServer_CheckPassword, netadr_t &adr, const char *password, const char *name)
	{
		if (StringStartsWith(password, "dest=")) {
			hasRedirect = true;
			const char *afterDest = strchr(password, '&');
			password = afterDest != nullptr ? afterDest+1 : password+5;
			if (redirect_only.GetBool()) return true;
		}
		else if (redirect_only.GetBool()) {
			return false;
		}
		return DETOUR_MEMBER_CALL(adr, password, name);
	}

    class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Crash_Fixes")
		{
			MOD_ADD_DETOUR_MEMBER(CBaseServer_ConnectClient, "CBaseServer::ConnectClient");

			MOD_ADD_DETOUR_MEMBER(CBaseServer_CheckPassword, "CBaseServer::CheckPassword");
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CServerGameClients_ClientPutInServer, "CServerGameClients::ClientPutInServer", HIGHEST);

			MOD_ADD_DETOUR_MEMBER(CServerGameDLL_GetServerBrowserMapOverride, "CServerGameDLL::GetServerBrowserMapOverride");
			
		}
	};
	CMod s_Mod;

    ConVar cvar_enable("sig_etc_server_redirects", "0", FCVAR_NOTIFY,
		"Allows for server redirects with passwords",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}