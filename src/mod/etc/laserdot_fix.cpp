#include "mod.h"
#include "util/rtti.h"


class C_SniperDot {};


namespace Mod::Etc::LaserDot_Fix
{
	DETOUR_DECL_MEMBER(void, C_SniperDot_ClientThink)
	{
#if 0
		DevMsg("C_SniperDot::ClientThink\n");
		
		auto rtti_unk = rtti_cast<IClientUnknown *>(reinterpret_cast<C_SniperDot *>(this));
		DevMsg("  rtti_unk %08x\n", (uintptr_t)rtti_unk);
		
		auto unk = reinterpret_cast<IClientUnknown *>(this);
		DevMsg("  unk %08x\n", (uintptr_t)unk);
		
		auto net = unk->GetClientNetworkable();
		DevMsg("  net %08x\n", (uintptr_t)net);
		
		auto cclass = net->GetClientClass();
		DevMsg("  cclass %08x\n", (uintptr_t)cclass);
		
		DevMsg("  name \"%s\"\n", cclass->GetName());
		
		// GetClientClass()->GetName() should be either:
		// "CLaserDot"
		// "CSniperDot"
		
		DETOUR_MEMBER_CALL();
#endif
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:LaserDot_Fix")
		{
			MOD_ADD_DETOUR_MEMBER(C_SniperDot_ClientThink, "[client] C_SniperDot::ClientThink");
		}

		virtual bool IsClientSide() override { return true; }
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_etc_laserdot_fix", "0", FCVAR_NOTIFY,
		"Mod: client-side fix for env_laserdot drawing the sniper laser particle",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
