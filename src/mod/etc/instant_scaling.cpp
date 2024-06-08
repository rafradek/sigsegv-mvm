#include "mod.h"
#include "stub/gamerules.h"
#include "stub/tfplayer.h"


namespace Mod::Etc::Instant_Scaling
{
	ConVar cvar_scalespeed  ("sig_mvm_body_scale_speed",   "1", FCVAR_NONE, "Body scale speed mult");

	DETOUR_DECL_MEMBER(void, CTFPlayer_TFPlayerThink)
	{
		DETOUR_MEMBER_CALL();
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (cvar_scalespeed.GetFloat() != 1.0f && TFGameRules()->IsMannVsMachineMode()) {
			player->m_flHeadScale = Approach(player->GetDesiredHeadScale(), player->m_flHeadScale, (cvar_scalespeed.GetFloat()-1) * gpGlobals->frametime);
			player->m_flHandScale = Approach(player->GetDesiredHandScale(), player->m_flHandScale, (cvar_scalespeed.GetFloat()-1) * gpGlobals->frametime);
			player->m_flTorsoScale = Approach(player->GetDesiredTorsoScale(), player->m_flTorsoScale, (cvar_scalespeed.GetFloat()-1) * gpGlobals->frametime);
		}
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Instant_Scaling")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_TFPlayerThink,  "CTFPlayer::TFPlayerThink");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_etc_instant_scaling", "0", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Mod: make hand/head/torso scaling instantaneous in MvM mode",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
