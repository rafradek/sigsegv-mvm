#include "mod.h"
#include "stub/baseentity.h"


namespace Mod::Etc::Medicgun_Beam_Machinery
{
	DETOUR_DECL_MEMBER(bool, CWeaponMedigun_AllowedToHealTarget, CBaseEntity *pTarget)
	{
		if (pTarget->IsBaseObject()) return true;
		
		return DETOUR_MEMBER_CALL(pTarget);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Medicgun_Beam_Machinery")
		{
			MOD_ADD_DETOUR_MEMBER(CWeaponMedigun_AllowedToHealTarget, "CWeaponMedigun::AllowedToHealTarget");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_etc_medicgun_beam_machinery", "0", FCVAR_NOTIFY,
		"Etc: enable CBaseObject entities as heal beam targets",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
