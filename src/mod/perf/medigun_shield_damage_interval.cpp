#ifdef SE_IS_TF2
#include "mod.h"
#include "stub/tf_shareddefs.h"
#include "stub/tfweaponbase.h"
#include "util/scope.h"
#include "stub/tfentities.h"
#include "util/rtti.h"
#include "re/nextbot.h"
#include "stub/tfbot.h"
#include "util/clientmsg.h"

namespace Mod::Perf::Medigun_Shield_Damage_Interval
{
	DETOUR_DECL_MEMBER(int, CBaseEntity_TakeDamage, const CTakeDamageInfo& inputInfo)
	{
		auto ent = reinterpret_cast<CBaseEntity *>(this);
		
		if (inputInfo.GetDamageCustom() == TF_DMG_CUSTOM_PLASMA) {
			auto medigun = rtti_cast<CWeaponMedigun *>(inputInfo.GetWeapon());
			
			if (medigun != nullptr) {
				extern ConVar cvar_enable;
				int interval = Max(cvar_enable.GetInt(), 1);
				
				if (interval > 1) {
					int victim_entindex = ENTINDEX(ent);
					
					if ((victim_entindex % interval) == (gpGlobals->tickcount % interval)) {
						CTakeDamageInfo newInfo = inputInfo;
						newInfo.ScaleDamage(interval);
						
						return DETOUR_MEMBER_CALL(CBaseEntity_TakeDamage)(newInfo);
					} else {
						return 0;
					}
				}
			}
		}
		
		
		return DETOUR_MEMBER_CALL(CBaseEntity_TakeDamage)(inputInfo);
	}

	class CMod : public IMod
	{
	public:
		CMod() : IMod("Perf:Medigun_Shield_Damage_Interval")
		{
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_TakeDamage, "CBaseEntity::TakeDamage");

		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_perf_medigun_shield_damage_interval", "0", FCVAR_NOTIFY,
		"Mod: change the medigun shield damage interval to values greater than every single tick",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
#endif