#include "mod.h"
#include "stub/tfbot.h"


namespace Mod::Pop::ExtAttr::AlwaysFireWeaponAlt
{
	DETOUR_DECL_MEMBER(void, CTFBotMainAction_FireWeaponAtEnemy, CTFBot *actor)
	{
		//DevMsg("DETOUR: CTFBotMainAction::FireWeaponAtEnemy\n");
		//DevMsg("  actor %08x\n", (uintptr_t)actor);
		actor->ExtAttr().Dump();
		
		if (actor != nullptr && actor->ExtAttr()[CTFBot::ExtendedAttr::ALWAYS_FIRE_WEAPON_ALT]) {
			//DevMsg("  actor has ExtAttr::ALWAYS_FIRE_WEAPON_ALT, pressing alt fire\n");
			actor->PressAltFireButton();
		}
		
		DETOUR_MEMBER_CALL(CTFBotMainAction_FireWeaponAtEnemy)(actor);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Pop:ExtAttr:AlwaysFireWeaponAlt")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_FireWeaponAtEnemy, "CTFBotMainAction::FireWeaponAtEnemy");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_pop_extattr_alwaysfireweaponalt", "0", FCVAR_NOTIFY,
		"Extended bot attr: AlwaysFireWeaponAlt",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
