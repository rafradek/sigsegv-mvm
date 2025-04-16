#include "mod.h"
#include "stub/baseplayer.h"
#include "util/clientmsg.h"


namespace Mod::Debug::Sticky_Detonation_Order
{
	int indent = 0;
	
	template<typename... ARGS>
	void ClientMsgAllIndent(const char *fmt, ARGS&&... args)
	{
		size_t fmt2_len = strlen(fmt) + (2 * indent) + 1;
		char *fmt2 = new char[fmt2_len];
		
		memset(fmt2, ' ', (2 * indent));
		V_strncpy(fmt2 + (2 * indent), fmt, fmt2_len);
		
		ClientMsgAll(fmt2, std::forward<ARGS>(args)...);
		
		delete[] fmt2;
	}
	
	
	DETOUR_DECL_MEMBER(int, CBaseEntity_TakeDamage, const CTakeDamageInfo& inputInfo)
	{
		ClientMsgAllIndent("CBaseEntity::TakeDamage\n");
		
		++indent;
		auto result = DETOUR_MEMBER_CALL(inputInfo);
		--indent;
		
		return result;
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFWeaponBaseGrenadeProj_Explode, trace_t *pTrace, int bitsDamageType)
	{
		ClientMsgAllIndent("BEGIN CTFWeaponBaseGrenadeProj::Explode\n");
		
		++indent;
		DETOUR_MEMBER_CALL(pTrace, bitsDamageType);
		--indent;
		
		ClientMsgAllIndent("END   CTFWeaponBaseGrenadeProj::Explode\n");
	}
	
	
	DETOUR_DECL_MEMBER(bool, CTFPipebombLauncher_DetonateRemotePipebombs, bool bFizzle)
	{
		ClientMsgAllIndent("BEGIN CTFPipebombLauncher::DetonateRemotePipebombs [bFizzle: %s]\n",
			(bFizzle ? "true" : "false"));
		
		++indent;
		auto result = DETOUR_MEMBER_CALL(bFizzle);
		--indent;
		
		ClientMsgAllIndent("END   CTFPipebombLauncher::DetonateRemotePipebombs [%s]\n",
			(result ? "true" : "false"));
		
		return result;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:Sticky_Detonation_Order")
		{
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_TakeDamage,                      "CBaseEntity::TakeDamage");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseGrenadeProj_Explode,            "CTFWeaponBaseGrenadeProj::Explode");
			MOD_ADD_DETOUR_MEMBER(CTFPipebombLauncher_DetonateRemotePipebombs, "CTFPipebombLauncher::DetonateRemotePipebombs");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_sticky_detonation_order", "0", FCVAR_NOTIFY,
		"Debug: diagnose weird effects due to stickybomb detonation order",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
