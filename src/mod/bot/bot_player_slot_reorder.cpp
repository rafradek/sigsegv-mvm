#include "mod.h"
#include "stub/baseplayer.h"
#include "stub/server.h"

namespace Mod::Etc::Extra_Player_Slots
{
	bool ExtraSlotsEnabledExternal();
}

namespace Mod::Bot::Bot_Player_Slot_Reorder
{

    RefCount rc_CBaseServer_CreateFakeClient;
    RefCount rc_CBaseServer_CreateFakeClient_HLTV;
    DETOUR_DECL_MEMBER(CBaseClient *, CBaseServer_CreateFakeClient, const char *name)
	{
        static ConVarRef tv_name("tv_name");
        SCOPED_INCREMENT(rc_CBaseServer_CreateFakeClient);
        SCOPED_INCREMENT_IF(rc_CBaseServer_CreateFakeClient_HLTV, tv_name.GetString() == name);
        
		return DETOUR_MEMBER_CALL(CBaseServer_CreateFakeClient)(name);
	}

    inline int GetHLTVSlot()
    {
        return gpGlobals->maxClients - 1;
    }

    DETOUR_DECL_MEMBER(CBaseClient *, CBaseServer_GetFreeClient, netadr_t &adr)
	{
        auto server = reinterpret_cast<CBaseServer *>(this);

		static ConVarRef tv_enable("tv_enable");
        if (server != hltv && rc_CBaseServer_CreateFakeClient) {
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

            int desiredSlot = GetHLTVSlot();
            if (!rc_CBaseServer_CreateFakeClient_HLTV) {
                desiredSlot = gpGlobals->maxClients - 1;
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
        if (client != nullptr) {
            if (!rc_CBaseServer_CreateFakeClient_HLTV && client->GetPlayerSlot() == gpGlobals->maxClients - 1 && tv_enable.GetBool()) {
                return nullptr;
            }
        }
        return client;
	}
    
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Bot:Bot_Player_Slot_Reorder")
		{
			MOD_ADD_DETOUR_MEMBER(CBaseServer_CreateFakeClient,"CBaseServer::CreateFakeClient");
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CBaseServer_GetFreeClient,   "CBaseServer::GetFreeClient", LOW);
        }
    };
	CMod s_Mod;
    
	ConVar cvar_enable("sig_bot_player_slot_reorder", "0", FCVAR_NOTIFY,
		"Mod: Reorder player slots for bots so that bots use last slots",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}