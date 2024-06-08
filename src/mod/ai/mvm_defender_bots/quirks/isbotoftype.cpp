#include "mod.h"
#include "stub/gamerules.h"
#include "stub/tfbot.h"
//#include "util/backtrace.h"


namespace Mod::AI::MvM_Defender_Bots
{
	RefCount rc_Quirk_IsBotOfType;
	DETOUR_DECL_MEMBER(int, CTFBot_GetBotType)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		if (rc_Quirk_IsBotOfType > 0) {
			if (TFGameRules()->IsMannVsMachineMode() && bot->GetTeamNumber() == TF_TEAM_RED) {
			//	DevMsg("QUIRK: CTFBot::IsBotOfType\n");
			//	BACKTRACE();
				
				return 0;
			}
		}
		
		return 1337;
	//	return DETOUR_MEMBER_CALL();
	}
	
	
	#define AddDetour_Member(detour, addr) \
		mod->AddDetour(new CDetour(addr, GET_MEMBER_CALLBACK(detour), GET_MEMBER_INNERPTR(detour)))
	#define AddDetour_Static(detour, addr) \
		mod->AddDetour(new CDetour(addr, GET_STATIC_CALLBACK(detour), GET_STATIC_INNERPTR(detour)))
	
	#define AddQuirk_IsBotOfType(addr) \
		mod->AddDetour(new CFuncCount(rc_Quirk_IsBotOfType, addr))
	
	void AddQuirks_IsBotOfType(IMod *mod)
	{
		AddDetour_Member(CTFBot_GetBotType, "CTFBot::GetBotType");
		
		/* QUIRK: red bots get robot teleport condition duration */
		AddQuirk_IsBotOfType("CTFPlayer::TeleportEffect");
	}
}
