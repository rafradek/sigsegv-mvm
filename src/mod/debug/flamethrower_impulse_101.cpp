#include "mod.h"


namespace Mod::Debug::Flamethrower_Impulse_101
{
	DETOUR_DECL_MEMBER(const char *, C_TFFlameThrower_FullCritChargedEffectName)
	{
		auto result = DETOUR_MEMBER_CALL();
		
		NDebugOverlay::Clear();
		NDebugOverlay::ScreenText(0.05f, 0.05f, CFmtStr("C_TFFlameThrower::FullCritChargedEffectName (this = 0x%08x)",
			(uintptr_t)this), 0xff, 0xff, 0xff, 0xff, 3600.0f);
		NDebugOverlay::ScreenText(0.05f, 0.10f, CFmtStr("returned \"%s\"",
			result), 0xff, 0xff, 0xff, 0xff, 3600.0f);
		
	//	return result;
		return "";
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:Flamethrower_Impulse_101")
		{
			MOD_ADD_DETOUR_MEMBER(C_TFFlameThrower_FullCritChargedEffectName, "[client] C_TFFlameThrower::FullCritChargedEffectName");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_flamethrower_impulse_101", "0", FCVAR_NOTIFY,
		"Debug: investigate why \"impulse 101\" makes medigun charge particle appear on flamethrowers",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
