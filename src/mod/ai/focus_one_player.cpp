#include "mod.h"
#include "stub/tfbot.h"


namespace Mod::AI::Focus_One_Player
{
	ConVar cvar_name("sig_ai_focus_one_player_name", "", FCVAR_NOTIFY,
		"Mod: name of the player to focus (can be a substring)");
	
	
	bool IsTheTarget(CTFPlayer *player)
	{
		if (player->GetTeamNumber() != TF_TEAM_RED)                               return false;
		if (V_stristr(player->GetPlayerName(), cvar_name.GetString()) == nullptr) return false;
		
		return true;
	}
	
	
	DETOUR_DECL_MEMBER(bool, CTFBotVision_IsIgnored, CBaseEntity *ent)
	{
		CTFPlayer *player = ToTFPlayer(ent);
		if (player != nullptr && IsTheTarget(player)) {
			return false;
		//	return DETOUR_MEMBER_CALL(ent);
		}
		
		return true;
	}
	
	
	/* make the targeted player known to all bots even if not in their ordinary
	 * vision range; and if they're a spy, reveal their identity */
	void NotifyBotsAboutTarget()
	{
		for (int i = 1; i <= gpGlobals->maxClients; ++i) {
			CTFPlayer *player = ToTFPlayer(UTIL_PlayerByIndex(i));
			if (player == nullptr || !IsTheTarget(player)) continue;
			
			for (int j = 1; j <= gpGlobals->maxClients; ++j) {
				CTFBot *bot = ToTFBot(UTIL_PlayerByIndex(j));
				if (bot == nullptr || bot->GetTeamNumber() != TF_TEAM_BLUE) continue;
				
				if (player->IsPlayerClass(TF_CLASS_SPY)) {
					bot->RealizeSpy(player);
				}
				bot->GetVisionInterface()->AddKnownEntity(player);
			}
		}
	}
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("AI:Focus_One_Player")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBotVision_IsIgnored, "CTFBotVision::IsIgnored");
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			static long frame = 0;
			++frame;
			
			if (frame % 7 == 0) {
				NotifyBotsAboutTarget();
			}
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_ai_focus_one_player", "0", FCVAR_NOTIFY,
		"Mod: make bots relentlessly attack one player and ignore everyone else",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
