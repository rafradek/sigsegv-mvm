#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/tf_shareddefs.h"
#include "util/scope.h"


namespace Mod::MvM::RoboSapper_Override
{
	ConVar cvar_radius("sig_mvm_robosapper_override_radius", "-1", FCVAR_NOTIFY,
		"Mod: robo sapper radius (usual: 0 @ L0, 200 @ L1, 225 @ L2, 250 @ L3); if negative, no override will be applied");
	ConVar cvar_duration("sig_mvm_robosapper_override_duration", "-1.0", FCVAR_NOTIFY,
		"Mod: robo sapper duration (usual: 4.0 @ L0, 4.0 @ L1, 5.5 @ L2, 7.0 @ L3); if negative, no override will be applied");
	
	ConVar cvar_stun_minibosses("sig_mvm_robosapper_override_stun_minibosses", "0", FCVAR_NOTIFY,
		"Mod: robo sapper will apply full stun to minibosses instead of just partial stun (usual: 0)");
	ConVar cvar_stun_amount("sig_mvm_robosapper_override_stun_amount", "0.85", FCVAR_NOTIFY,
		"Mod: robo sapper will apply specified stun amount (usual: 0.85)");
	
	
	DETOUR_DECL_MEMBER(void, CObjectSapper_ApplyRoboSapper, CTFPlayer *target, float duration, int radius)
	{
		if (cvar_duration.GetFloat() >= 0.0f) {
			duration = cvar_duration.GetFloat();
		}
		
		if (cvar_radius.GetInt() >= 0) {
			radius = cvar_radius.GetInt();
		}
		
		DETOUR_MEMBER_CALL(CObjectSapper_ApplyRoboSapper)(target, duration, radius);
	}
	
	RefCount rc_CObjectSapper_ApplyRoboSapperEffects;
	DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, bool, CObjectSapper_ApplyRoboSapperEffects, CTFPlayer *target, float duration)
	{
		SCOPED_INCREMENT(rc_CObjectSapper_ApplyRoboSapperEffects);
		return DETOUR_MEMBER_CALL(CObjectSapper_ApplyRoboSapperEffects)(target, duration);
	}
	
	DETOUR_DECL_MEMBER(void, CTFPlayerShared_StunPlayer, float duration, float slowdown, int flags, CTFPlayer *attacker)
	{
		auto shared = reinterpret_cast<CTFPlayerShared *>(this);
		
		if (rc_CObjectSapper_ApplyRoboSapperEffects > 0) {
			CTFPlayer *player = shared->GetOuter();
			
			if (cvar_stun_minibosses.GetBool() && player->IsMiniBoss()) {
				flags = (TF_STUNFLAG_THIRDPERSON | TF_STUNFLAG_BONKSTUCK | TF_STUNFLAG_SLOWDOWN);
			}
			
			slowdown = cvar_stun_amount.GetFloat();
		}
		
		DETOUR_MEMBER_CALL(CTFPlayerShared_StunPlayer)(duration, slowdown, flags, attacker);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:RoboSapper_Override")
		{
			MOD_ADD_DETOUR_MEMBER(CObjectSapper_ApplyRoboSapper,        "CObjectSapper::ApplyRoboSapper");
			MOD_ADD_DETOUR_MEMBER(CObjectSapper_ApplyRoboSapperEffects, "CObjectSapper::ApplyRoboSapperEffects [clone]");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_StunPlayer,           "CTFPlayerShared::StunPlayer");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_robosapper_override", "0", FCVAR_NOTIFY,
		"Mod: enable overriding aspects of the robo sapper in MvM mode",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
