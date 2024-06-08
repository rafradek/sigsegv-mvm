#include "mod.h"


namespace Mod::Attr::Undocumented
{
	ConVar cvar_parachute_disable("sig_attr_undocumented_parachute_disable", "0", FCVAR_NOTIFY,
		"");
	ConVar cvar_fires_milk_bolts("sig_attr_undocumented_fires_milk_bolts", "0", FCVAR_NOTIFY,
		"");
	ConVar cvar_force_distribute_currency_on_death("sig_attr_undocumented_force_distribute_currency_on_death", "0", FCVAR_NOTIFY,
		"");
	
	
	DETOUR_DECL_STATIC(float, CAttributeManager_ApplyAttributeFloatWrapper, float val, CBaseEntity *ent, string_t name, CUtlVector<CBaseEntity *> *vec)
	{
		const char *attr = STRING(name);
		
		if (strcmp(attr, "parachute_disable") == 0) {
			return cvar_parachute_disable.GetFloat();
		}
		if (strcmp(attr, "fires_milk_bolts") == 0) {
			return cvar_fires_milk_bolts.GetFloat();
		}
		if (strcmp(attr, "force_distribute_currency_on_death") == 0) {
			return cvar_force_distribute_currency_on_death.GetFloat();
		}
		
		return DETOUR_STATIC_CALL(val, ent, name, vec);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Attr:Undocumented")
		{
			MOD_ADD_DETOUR_STATIC(CAttributeManager_ApplyAttributeFloatWrapper, "CAttributeManager::ApplyAttributeFloatWrapper");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_attr_undocumented", "0", FCVAR_NOTIFY,
		"Mod: enable undocumented attribute classes",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
