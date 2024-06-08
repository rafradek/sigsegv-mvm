#include "mod.h"
#include "stub/nav.h"
#include "stub/tfbot.h"


namespace Mod::Debug::Func_Nav_Cost_Tags
{
	DETOUR_DECL_MEMBER(bool, CFuncNavCost_IsApplicableTo, CBaseCombatCharacter *who)
	{
		auto func = reinterpret_cast<CFuncNavCost *>(this);
		
		auto result = DETOUR_MEMBER_CALL(who);
		
		Msg("\nCFuncNavCost::IsApplicableTo: %s #%d \"%s\"\n",
			func->GetClassname(), ENTINDEX(func), STRING(func->GetEntityName()));
		
		Msg("  this->m_team:       %d\n", (int)func->m_team);
		Msg("  this->m_isDisabled: %s\n", (func->m_isDisabled ? "true" : "false"));
		Msg("  this->m_iszTags:    \"%s\"\n", STRING((string_t)func->m_iszTags));
		
		char strTags[0x1000];
		V_strcpy_safe(strTags, "  this->m_tags:      ");
		for (const auto& tag : func->m_tags) {
			V_strcat_safe(strTags, " \"");
			V_strcat_safe(strTags, tag.Get());
			V_strcat_safe(strTags, "\"");
		}
		Msg("%s\n", strTags);
		
		CTFBot *bot = ToTFBot(who);
		if (bot != nullptr) {
			Msg("who: #%d \"%s\"\n", ENTINDEX(bot), bot->GetPlayerName());
			Msg("  who->GetTeamNumber(): %d\n", bot->GetTeamNumber());
			Msg("  who->HasTheFlag():    %s\n", (bot->HasTheFlag() ? "true" : "false"));
			Msg("  who->GetMission():    %d\n", bot->GetMission());
			
			Msg("  who->m_Tags.Count():  %d\n", bot->m_Tags->Count());
			
		//	V_strcpy_safe(strTags, "  who->m_Tags:         ");
		//	for (const auto& tag : (CUtlVector<CFmtStr>&)bot->m_Tags) {
		//		V_strcat_safe(strTags, " \"");
		//		V_strcat_safe(strTags, tag.Get());
		//		V_strcat_safe(strTags, "\"");
		//	}
		//	Msg("%s\n", strTags);
			
		//	Msg("  who->GetPlayerClass()->GetName(): \"%s\"\n", bot->GetPlayerClass()->GetName());
		} else {
			Msg("who: #%d (not a TFBot!)\n", ENTINDEX(who));
		}
		
		Msg("  result: %s\n", (result ? "true" : "false"));
		
		return result;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:Func_Nav_Cost_Tags")
		{
			MOD_ADD_DETOUR_MEMBER(CFuncNavCost_IsApplicableTo, "CFuncNavCost::IsApplicableTo");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_func_nav_cost_tags", "0", FCVAR_NOTIFY,
		"Debug: show information for func_nav_cost tag matching",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
