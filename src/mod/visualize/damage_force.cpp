#include "mod.h"
#include "stub/tfplayer.h"


namespace Mod::Visualize::Damage_Force
{
	DETOUR_DECL_MEMBER(int, CTFPlayer_OnTakeDamage, const CTakeDamageInfo& info)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		Vector vecCenter = player->WorldSpaceCenter();
		Vector vecForce  = info.GetDamageForce();
		
		if (vecForce != vec3_origin) {
			NDebugOverlay::HorzArrow(vecCenter, vecCenter + vecForce, 5.0f, 0xff, 0x00, 0x00, 0xff, true, 1.0f);
			NDebugOverlay::VertArrow(vecCenter, vecCenter + vecForce, 5.0f, 0xff, 0x00, 0x00, 0xff, true, 1.0f);
		}
		
		return DETOUR_MEMBER_CALL(info);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Visualize:Damage_Force")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_OnTakeDamage, "CTFPlayer::OnTakeDamage");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_visualize_damage_force", "0", FCVAR_NOTIFY,
		"Visualization: draw force vector from damage events",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
