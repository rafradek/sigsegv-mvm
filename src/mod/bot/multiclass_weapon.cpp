#include "mod.h"
#include "stub/tfbot.h"
#include "util/scope.h"
#include "stub/projectiles.h"

namespace Mod::Bot::MultiClass_Weapon
{
	/* a less strict version of TranslateWeaponEntForClass (don't return empty strings) */

	
	
	
	
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Bot:MultiClass_Weapon")
		{
			
			//MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_SecondaryAttack,     "CTFWeaponBase::SecondaryAttack");
			//MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_CanPerformSecondaryAttack,     "CTFWeaponBase::CanPerformSecondaryAttack");
			//MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_FireRocket,     "CTFWeaponBaseGun::FireRocket");
			//MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGun_FireFlare,     "CTFWeaponBaseGun::FireFlare");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_bot_multiclass_weapon", "0", FCVAR_NOTIFY,
		"Mod: remap item entity names so bots can be given multi-class weapons",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
