#include "mod.h"


namespace Mod::Debug::NoKnockback
{
	DETOUR_DECL_MEMBER(void, CTFPlayer_ApplyPushFromDamage, const CTakeDamageInfo& info, Vector v1)
	{
		/* do nothing */
	}
	
	
	ConVar cvar_airblast("sig_debug_noknockback_airblast", "0", FCVAR_NOTIFY,
		"Debug: also disable ApplyGenericPushbackImpulse");
	
	DETOUR_DECL_MEMBER(void, CTFPlayer_ApplyGenericPushbackImpulse, const Vector& impulse, CTFPlayer * inflictor)
	{
		if (!cvar_airblast.GetBool()) {
			DETOUR_MEMBER_CALL(CTFPlayer_ApplyGenericPushbackImpulse)(impulse, inflictor);
		}
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:NoKnockback")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ApplyPushFromDamage, "CTFPlayer::ApplyPushFromDamage");
			
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ApplyGenericPushbackImpulse, "CTFPlayer::ApplyGenericPushbackImpulse");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_noknockback", "0", FCVAR_NOTIFY,
		"Debug: disable all damage-induced knockback effects",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
