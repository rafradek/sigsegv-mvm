#include "mod.h"
#include "stub/tfbot.h"
#include "stub/gamerules.h"
//#include "util/backtrace.h"


namespace Mod::AI::MvM_Defender_Bots
{
	RefCount rc_Quirk_GetLastKnownArea;
	DETOUR_DECL_MEMBER(CNavArea *, CTFBot_GetLastKnownArea)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		if (rc_Quirk_GetLastKnownArea > 0) {
			if (TFGameRules()->IsMannVsMachineMode() && bot->GetTeamNumber() == TF_TEAM_RED) {
			//	DevMsg("QUIRK: CTFBot::GetLastKnownArea\n");
			//	BACKTRACE();
				
				return nullptr;
			}
		}
		
		return DETOUR_MEMBER_CALL();
	}
	
	
	#define AddDetour_Member(detour, addr) \
		mod->AddDetour(new CDetour(addr, GET_MEMBER_CALLBACK(detour), GET_MEMBER_INNERPTR(detour)))
	#define AddDetour_Static(detour, addr) \
		mod->AddDetour(new CDetour(addr, GET_STATIC_CALLBACK(detour), GET_STATIC_INNERPTR(detour)))
	
	#define AddQuirk_GetLastKnownArea(addr) \
		mod->AddDetour(new CFuncCount(rc_Quirk_GetLastKnownArea, addr))
	
	void AddQuirks_GetLastKnownArea(IMod *mod)
	{
		AddDetour_Member(CTFBot_GetLastKnownArea, "CTFBot::GetLastKnownArea");
		
		/* QUIRK: red bots obey CanBotsAttackWhileInSpawnRoom */
		AddQuirk_GetLastKnownArea("CTFBotMainAction::ShouldAttack");
		AddQuirk_GetLastKnownArea("CTFBotMainAction::ShouldHurry");
		AddQuirk_GetLastKnownArea("CTFBotSniperLurk::ShouldAttack");
	}
}
