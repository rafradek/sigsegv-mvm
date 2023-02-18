#include "mod.h"
#include "stub/tfentities.h"
#include "util/scope.h"


namespace Mod::Debug::Tank_Destroy_Animation
{
	ConVar cvar_override("sig_debug_tank_destroy_animation_override", "", FCVAR_NOTIFY,
		"Debug: specify sequence to use for tank destruction");
	
	
	RefCount rc_CTFTankDestruction_Spawn;
	DETOUR_DECL_MEMBER(void, CTFTankDestruction_Spawn)
	{
		SCOPED_INCREMENT(rc_CTFTankDestruction_Spawn);
		DETOUR_MEMBER_CALL(CTFTankDestruction_Spawn)();
	}
	
	
	DETOUR_DECL_MEMBER(int, CBaseAnimating_LookupSequence, const char *label)
	{
		if (rc_CTFTankDestruction_Spawn > 0) {
			return DETOUR_MEMBER_CALL(CBaseAnimating_LookupSequence)(cvar_override.GetString());
		}
		
		return DETOUR_MEMBER_CALL(CBaseAnimating_LookupSequence)(label);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:Tank_Destroy_Animation")
		{
			MOD_ADD_DETOUR_MEMBER(CTFTankDestruction_Spawn,      "CTFTankDestruction::Spawn");
			MOD_ADD_DETOUR_MEMBER(CBaseAnimating_LookupSequence, "CBaseAnimating::LookupSequence");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_tank_destroy_animation", "0", FCVAR_NOTIFY,
		"Debug: override tank destroy animation",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
