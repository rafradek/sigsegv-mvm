#include "mod.h"


namespace Mod::Debug::Client_Desync
{
	ConVar cvar_player_index("sig_debug_client_desync_index", "2", FCVAR_NOTIFY,
		"Entity index to track");
	
	
	const char *DataUpdateType_ToString(DataUpdateType_t updateType)
	{
		static char buf[1024];
		
		switch (updateType) {
		case DATA_UPDATE_CREATED:
			snprintf(buf, sizeof(buf), "DATA_UPDATE_CREATED");
			break;
		case DATA_UPDATE_DATATABLE_CHANGED:
			snprintf(buf, sizeof(buf), "DATA_UPDATE_DATATABLE_CHANGED");
			break;
		default:
			snprintf(buf, sizeof(buf), "??? (%d)", updateType);
			break;
		}
		
		return buf;
	}
	
	
	int n_prechanged = 0;
	int n_changed    = 0;
	
	
	DETOUR_DECL_MEMBER(void, C_TFPlayer_OnPreDataChanged, DataUpdateType_t updateType)
	{
		auto player = reinterpret_cast<IClientNetworkable *>(this);
		if (player->entindex() == cvar_player_index.GetInt()) {
			n_changed = 0;
			++n_prechanged;
			
			DevMsg("[%6.3f] #%-2d C_TFPlayer::OnPreDataChanged %d %s\n",
				Plat_FloatTime(), player->entindex(), n_prechanged, DataUpdateType_ToString(updateType));
			
		}
		
		DETOUR_MEMBER_CALL(updateType);
	}
	
	DETOUR_DECL_MEMBER(void, C_TFPlayer_OnDataChanged, DataUpdateType_t updateType)
	{
		auto player = reinterpret_cast<IClientNetworkable *>(this);
		if (player->entindex() == cvar_player_index.GetInt()) {
			n_prechanged = 0;
			++n_changed;
			
			DevMsg("[%6.3f] #%-2d C_TFPlayer::OnDataChanged    %d %s\n",
				Plat_FloatTime(), player->entindex(), n_changed, DataUpdateType_ToString(updateType));
		}
		
		DETOUR_MEMBER_CALL(updateType);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:Client_Desync")
		{
			MOD_ADD_DETOUR_MEMBER(C_TFPlayer_OnPreDataChanged, "[client] C_TFPlayer::OnPreDataChanged");
			MOD_ADD_DETOUR_MEMBER(C_TFPlayer_OnDataChanged,    "[client] C_TFPlayer::OnDataChanged");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_client_desync", "0", FCVAR_NOTIFY,
		"Debug: diagnose client entity state desynchronization",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}

/*
20160327? win

0x1c10 C_TFPlayer::m_bArenaSpectator
0x1c11 C_TFPlayer::m_bIsMiniBoss
0x1c12 C_TFPlayer::m_bIsABot
0x1c13 
0x1c14 C_TFPlayer::m_nBotSkill
0x1c15 C_TFPlayer::m_nBotSkill [old version saved in OnPreDataChanged]

0x2f1c C_TFPlayer::m_bUseBossHealthBar


20160319a osx
C_TFPlayer::m_bIsMiniBoss @ 0x1bed
*/


/* 20160202

C_TFPlayer::m_bUseBossHealthBar @ 0x2ee8
- C_TFPlayer::GetBossProgressImageName (vt+0x5dc)

C_TFPlayer::m_bIsMiniBoss @ 0x1bed

C_TFPlayer::m_nBotSkill @ 0x1bf0
- old value @ 0x1bf4

C_BaseAninating::m_flModelScale @ 0x648

*/
