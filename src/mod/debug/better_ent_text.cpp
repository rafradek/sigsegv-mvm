#include "mod.h"
#include "stub/baseentity.h"
#include "stub/tf_shareddefs.h"
#include "re/nextbot.h"


namespace Mod::Debug::Better_Ent_Text
{
	const char *GetTeamNameString(int teamnum)
	{
		switch (teamnum) {
		case TEAM_ANY:        return "TEAM_ANY";
		case TEAM_INVALID:    return "TEAM_INVALID";
		case TEAM_UNASSIGNED: return "TEAM_UNASSIGNED";
		case TEAM_SPECTATOR:  return "TEAM_SPECATATOR";
		case TF_TEAM_RED:     return "TF_TEAM_RED";
		case TF_TEAM_BLUE:    return "TF_TEAM_BLUE";
		default:              return "???";
		}
	}
	
	
	DETOUR_DECL_MEMBER(int, CBaseEntity_DrawDebugTextOverlays)
	{
		auto ent = reinterpret_cast<CBaseEntity *>(this);
		
		int line = DETOUR_MEMBER_CALL();
		
		if ((ent->m_debugOverlays & OVERLAY_TEXT_BIT) != 0) {
			ent->EntityText(line++, CFmtStr("Team: %d %s", ent->GetTeamNumber(), GetTeamNameString(ent->GetTeamNumber())), 0.0f, 0xff, 0xff, 0xff, 0xff);
			
			INextBot *nextbot = ent->MyNextBotPointer();
			if (nextbot != nullptr) {
				ent->EntityText(line++, CFmtStr("Speed: %.1f HU/s", nextbot->GetLocomotionInterface()->GetVelocity().Length()), 0.0f, 0xff, 0xff, 0xff, 0xff);
			} else {
				ent->EntityText(line++, CFmtStr("Speed: %.1f HU/s", ent->GetAbsVelocity().Length()), 0.0f, 0xff, 0xff, 0xff, 0xff);
			}
		}
		
		return line;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:Better_Ent_Text")
		{
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_DrawDebugTextOverlays, "CBaseEntity::DrawDebugTextOverlays");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_better_ent_text", "0", FCVAR_NOTIFY,
		"Debug: make the ent_text command less useless",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
