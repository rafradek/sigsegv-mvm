#include "mod.h"
#include "stub/gamerules.h"
#include "stub/tfplayer.h"
#include "stub/tfweaponbase.h"
#include "stub/tf_shareddefs.h"
#include "stub/objects.h"
#include "stub/trace.h"
#include "stub/misc.h"


namespace Mod::Debug::Melee_Trace
{
	/* included within this namespace */
	#include "mod/debug/melee_trace_normal.h"
	#include "mod/debug/melee_trace_improved.h"
	
	
	ConVar cvar_mode("sig_debug_melee_trace_mode", "valve", FCVAR_NOTIFY,
		"Debug: method to use for swing trace: [valve|normal|improved]");
	
	DETOUR_DECL_MEMBER(bool, CTFWeaponBaseMelee_DoSwingTraceInternal, trace_t& trace, bool bCleaveAttack, CUtlVector<trace_t> *pTraces)
	{
		if (V_stricmp(cvar_mode.GetString(), "normal") == 0) {
			auto weapon = reinterpret_cast<Normal::CTFWeaponBaseMeleeExt *>(this);
			return weapon->DoSwingTraceInternal(trace, bCleaveAttack, pTraces);
		}
		
		if (V_stricmp(cvar_mode.GetString(), "improved") == 0) {
			auto weapon = reinterpret_cast<Improved::CTFWeaponBaseMeleeExt *>(this);
			return weapon->DoSwingTraceInternal(trace, bCleaveAttack, pTraces);
		}
		
		return DETOUR_MEMBER_CALL(trace, bCleaveAttack, pTraces);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:Melee_Trace")
		{
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseMelee_DoSwingTraceInternal, "CTFWeaponBaseMelee::DoSwingTraceInternal");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_melee_trace", "0", FCVAR_NOTIFY,
		"Debug: investigate ways to improve melee hit detection",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
