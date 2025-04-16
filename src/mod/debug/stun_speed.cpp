#include "mod.h"
#include "stub/tfbot.h"


namespace Mod::Debug::Stun_Speed
{
	int Common_DrawDebugTextOverlays(CTFPlayer *player)
	{
		int line = 1;
		player->EntityText(line++, CFmtStr("Speed: %6.1f HU/s", player->GetAbsVelocity().Length()), 0.0f, 0xff, 0xff, 0xff, 0xff);
		
		return line;
	}
	
	
	DETOUR_DECL_MEMBER(int, CTFBot_DrawDebugTextOverlays)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		return Common_DrawDebugTextOverlays(bot);
	}
	
	DETOUR_DECL_MEMBER(int, CTFPlayer_DrawDebugTextOverlays)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		return Common_DrawDebugTextOverlays(player);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:Stun_Speed")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBot_DrawDebugTextOverlays,    "CTFBot::DrawDebugTextOverlays");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_DrawDebugTextOverlays, "CTFPlayer::DrawDebugTextOverlays");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_stun_speed", "0", FCVAR_NOTIFY,
		"Debug: show speed of stunned players/bots",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
