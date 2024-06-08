#include "mod.h"
#include "stub/tfweaponbase.h"
#include "stub/tfbot.h"
#include "util/scope.h"


namespace Mod::AI::WrapAssassin_AltFire
{
	RefCount rc_CTFBot_OpportunisticallyUseWeaponAbilities;
	DETOUR_DECL_MEMBER(Action<CTFBot> *, CTFBot_OpportunisticallyUseWeaponAbilities)
	{
	//	DevMsg("CTFBot::OpportunisticallyUseWeaponAbilities BEGIN\n");
		SCOPED_INCREMENT(rc_CTFBot_OpportunisticallyUseWeaponAbilities);
		auto result = DETOUR_MEMBER_CALL();
	//	DevMsg("CTFBot::OpportunisticallyUseWeaponAbilities END\n");
		return result;
	}
	
	DETOUR_DECL_MEMBER(int, CTFBat_Giftwrap_GetWeaponID)
	{
		if (rc_CTFBot_OpportunisticallyUseWeaponAbilities > 0) {
		//	DevMsg("  CTFBat_Giftwrap::GetWeaponID PASSTHRU\n");
			return TF_WEAPON_BAT_WOOD;
		}
		
	//	DevMsg("  CTFBat_Giftwrap::GetWeaponID PASSTHRU\n");
		return DETOUR_MEMBER_CALL();
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("AI:WrapAssassin_AltFire")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBot_OpportunisticallyUseWeaponAbilities, "CTFBot::OpportunisticallyUseWeaponAbilities");
			MOD_ADD_DETOUR_MEMBER(CTFBat_Giftwrap_GetWeaponID,                "CTFBat_Giftwrap::GetWeaponID");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_ai_wrapassassin_altfire", "0", FCVAR_NOTIFY,
		"Mod: make bots fire Wrap Assassin balls, similar to how they use the Sandman",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
