#include "mod.h"
#include "stub/gamerules.h"


namespace Mod::Etc::Instant_Scaling
{
	ConVar cvar_scalespeed  ("sig_mvm_body_scale_speed",   "1", FCVAR_NONE, "Body scale speed mult");
	DETOUR_DECL_MEMBER(float, CTFPlayer_GetHandScaleSpeed)
	{
		if (TFGameRules()->IsMannVsMachineMode()) {
			return DETOUR_MEMBER_CALL(CTFPlayer_GetHandScaleSpeed)() * cvar_scalespeed.GetFloat();
		}
		
		return DETOUR_MEMBER_CALL(CTFPlayer_GetHandScaleSpeed)();
	}
	
	DETOUR_DECL_MEMBER(float, CTFPlayer_GetHeadScaleSpeed)
	{
		if (TFGameRules()->IsMannVsMachineMode()) {
			return DETOUR_MEMBER_CALL(CTFPlayer_GetHeadScaleSpeed)() * cvar_scalespeed.GetFloat();
		}
		
		return DETOUR_MEMBER_CALL(CTFPlayer_GetHeadScaleSpeed)();
	}
	
	DETOUR_DECL_MEMBER(float, CTFPlayer_GetTorsoScaleSpeed)
	{
		if (TFGameRules()->IsMannVsMachineMode()) {
			return DETOUR_MEMBER_CALL(CTFPlayer_GetTorsoScaleSpeed)() * cvar_scalespeed.GetFloat();
		}
		
		return DETOUR_MEMBER_CALL(CTFPlayer_GetTorsoScaleSpeed)();
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Instant_Scaling")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetHandScaleSpeed,  "CTFPlayer::GetHandScaleSpeed");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetHeadScaleSpeed,  "CTFPlayer::GetHeadScaleSpeed");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetTorsoScaleSpeed, "CTFPlayer::GetTorsoScaleSpeed");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_etc_instant_scaling", "0", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Mod: make hand/head/torso scaling instantaneous in MvM mode",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
