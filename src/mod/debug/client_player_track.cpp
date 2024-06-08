#include "mod.h"


namespace Mod::Client::Player_Track
{
	// can we find NDebugOverlay::EntityBounds in the client?
	
	
	ConVar cvar_indexes("sig_debug_client_player_track_indexes", "all", FCVAR_NOTIFY,
		"Debug: list of entity indexes to track (or 'all')");
	ConVar cvar_duration("sig_debug_client_player_track_duration", "0.0167", FCVAR_NOTIFY,
		"Debug: duration for the overlay");
	
	
	void TrackPlayers()
	{
		if (debugoverlay == nullptr) {
			Warning("debugoverlay is nullptr!\n");
			return;
		}
		
		if (V_stricmp(cvar_indexes.GetString(), "all") == 0) {
			for (int i = 1; i <= 32; ++i) {
				debugoverlay->AddEntityTextOverlay(i, 0, cvar_duration.GetFloat(), 0xff, 0xff, 0xff, 0xff, "#%d", i);
			}
		} else {
			char *dup = strdup(cvar_indexes.GetString());
			
			char *s = strtok(dup, ",");
			while (s != nullptr) {
				int idx = std::stoi(s, nullptr, 0);
				
			//	DevMsg("%d\n", idx);
				if (idx >= 0 && idx < 2048) {
					debugoverlay->AddEntityTextOverlay(idx, 0, cvar_duration.GetFloat(), 0xff, 0xff, 0xff, 0xff, "#%d", idx);
				}
				
				s = strtok(nullptr, ",");
			}
			
			free(dup);
		}
	}
	
	
	DETOUR_DECL_MEMBER(void, CViewRender_Render, vrect_t *rect)
	{
	//	DevMsg("CViewRender::Render\n");
		
		TrackPlayers();
		
		DETOUR_MEMBER_CALL(rect);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:Client_Player_Track")
		{
			MOD_ADD_DETOUR_MEMBER(CViewRender_Render, "[client] CViewRender::Render");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_client_player_track", "0", FCVAR_NOTIFY,
		"Debug: enable tracking players on the client side during demo playback",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
