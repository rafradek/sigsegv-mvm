#include "mod.h"
#include "stub/gamerules.h"
#include "stub/tfbot.h"
//#include "util/backtrace.h"


namespace Mod::AI::MvM_Defender_Bots
{
	RefCount rc_Quirk_IsFakeClient;
	DETOUR_DECL_MEMBER(bool, NextBotPlayer_CTFPlayer_IsFakeClient)
	{
		auto player = reinterpret_cast<NextBotPlayer<CTFPlayer> *>(this);
		
		if (rc_Quirk_IsFakeClient > 0) {
			if (TFGameRules()->IsMannVsMachineMode() && player->GetTeamNumber() == TF_TEAM_RED) {
			//	DevMsg("QUIRK: NextBotPlayer<CTFPlayer>::IsFakeClient\n");
			//	BACKTRACE();
				
				return false;
			}
		}
		
		return true;
	//	return DETOUR_MEMBER_CALL();
	}
	
	
	#define AddDetour_Member(detour, addr) \
		mod->AddDetour(new CDetour(addr, GET_MEMBER_CALLBACK(detour), GET_MEMBER_INNERPTR(detour)))
	#define AddDetour_Static(detour, addr) \
		mod->AddDetour(new CDetour(addr, GET_STATIC_CALLBACK(detour), GET_STATIC_INNERPTR(detour)))
	
	#define AddQuirk_IsFakeClient(addr) \
		mod->AddDetour(new CFuncCount(rc_Quirk_IsFakeClient, addr))
	
	void AddQuirks_IsFakeClient(IMod *mod)
	{
		AddDetour_Member(NextBotPlayer_CTFPlayer_IsFakeClient, "NextBotPlayer<CTFPlayer>::IsFakeClient");
		
		/* QUIRK: bots are skipped in deciding whether to allow readying up */
		AddQuirk_IsFakeClient("CTFGameRules::PlayerReadyStatus_HaveMinPlayersToEnable");
	}
}
