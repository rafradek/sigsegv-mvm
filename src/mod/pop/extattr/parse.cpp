#include "mod.h"
#include "stub/tfbot.h"


namespace Mod::Pop::ExtAttr::Parse
{
	#warning __gcc_regcall detours considered harmful!
	DETOUR_DECL_STATIC_CALL_CONVENTION(__gcc_regcall, bool, ParseDynamicAttributes, void *ecattr, KeyValues *kv)
	{
		//DevMsg("ParseDynamicAttributes: \"%s\" \"%s\"\n", kv->GetName(), kv->GetString());
		
		if (V_stricmp(kv->GetName(), "ExtAttr") == 0) {
			auto ext = reinterpret_cast<CTFBot::ExtendedAttr *>((uintptr_t)ecattr + 0x10);
			
			const char *val = kv->GetString();
			if (V_stricmp(val, "AlwaysFireWeaponAlt") == 0) {
				DevMsg("  found: ExtAttr AlwaysFireWeaponAlt\n");
				ext->TurnOn(CTFBot::ExtendedAttr::ALWAYS_FIRE_WEAPON_ALT);
			} else if (V_stricmp(val, "TargetStickies") == 0) {
				DevMsg("  found: ExtAttr TargetStickies\n");
				ext->TurnOn(CTFBot::ExtendedAttr::TARGET_STICKIES);
			} else if (V_stricmp(val, "BuildDispenserAsSentryGun") == 0) {
				DevMsg("  found: ExtAttr BuildDispenserAsSentryGun\n");
				ext->TurnOn(CTFBot::ExtendedAttr::BUILD_DISPENSER_SG);
			} else if (V_stricmp(val, "BuildDispenserAsTeleporter") == 0) {
				DevMsg("  found: ExtAttr BuildDispenserAsTeleporter\n");
				ext->TurnOn(CTFBot::ExtendedAttr::BUILD_DISPENSER_TP);
			} else {
				Warning("TFBotSpawner: Invalid extended attribute '%s'\n", val);
			}
			
			DevMsg("  Got ExtAttr, returning true\n");
			return true;
		}
		
		//DevMsg("  Passing through to actual ParseDynamicAttributes\n");
		return DETOUR_STATIC_CALL(ParseDynamicAttributes)(ecattr, kv);
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFBot_OnEventChangeAttributes, void *ecattr)
	{
		if (this != nullptr && ecattr != nullptr) {
			auto bot = reinterpret_cast<CTFBot *>(this);
			auto ext = reinterpret_cast<CTFBot::ExtendedAttr *>((uintptr_t)ecattr + 0x10);
			
			bot->ExtAttr() = *ext;
		}
		
		DETOUR_MEMBER_CALL(CTFBot_OnEventChangeAttributes)(ecattr);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Pop:ExtAttr:Parse")
		{
			MOD_ADD_DETOUR_STATIC(ParseDynamicAttributes,         "ParseDynamicAttributes");
			MOD_ADD_DETOUR_MEMBER(CTFBot_OnEventChangeAttributes, "CTFBot::OnEventChangeAttributes");
		}
		
		// TODO: should we clear the ext attr map at
		// - mod load?
		// - mod unload?
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_pop_extattr_parse", "0", FCVAR_NOTIFY,
		"Mod: enable parsing of mod-specific extended bot attributes in MvM pop files",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
