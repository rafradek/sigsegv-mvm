#include "mod.h"
#include "stub/baseplayer.h"
#include "util/iterate.h"


namespace Mod::Util::Entity_Overlays
{
	/* sigh... */
	CBasePlayer *GetLocalPlayer()
	{
		static ConVarRef index("sig_util_listenserverhost_index");
		return UTIL_PlayerByIndex(index.GetInt());
	}
	
	
	DETOUR_DECL_STATIC(void, DrawAllDebugOverlays)
	{
		ForEachEntity([](CBaseEntity *ent){
			MDLCACHE_CRITICAL_SECTION();
			ent->DrawDebugGeometryOverlays();
		});
	}
	
	
	DETOUR_DECL_STATIC(void, SetDebugBits, CBasePlayer *pPlayer, const char *name, int bit)
	{
		if (pPlayer == nullptr) {
			pPlayer = GetLocalPlayer();
		}
		
		DevMsg("SetDebugBits([pPlayer: #%d] [name: \"%s\"] [bit: %08x]\n",
			ENTINDEX(pPlayer), name, bit);
		
		DETOUR_STATIC_CALL(pPlayer, name, bit);
	}
	
	
	void GetEntityDescStr(char *buf, size_t len, CBaseEntity *ent)
	{
		if (ent != nullptr) {
			V_snprintf(buf, len, "#%d %s \"%s\"", ENTINDEX(ent), ent->GetClassname(), STRING(ent->GetEntityName()));
		} else {
			V_snprintf(buf, len, "nullptr");
		}
	}
	
	DETOUR_DECL_STATIC(CBaseEntity *, GetNextCommandEntity, CBasePlayer *pPlayer, const char *name, CBaseEntity *ent)
	{
		auto result = DETOUR_STATIC_CALL(pPlayer, name, ent);
		
		char str_ent   [0x1000];
		char str_result[0x1000];
		
		GetEntityDescStr(str_ent,    sizeof(str_ent),    ent);
		GetEntityDescStr(str_result, sizeof(str_result), result);
		
		DevMsg("GetNextCommandEntity([pPlayer: #%d] [name: \"%s\"] [ent: %s]) = [%s]\n",
			ENTINDEX(pPlayer), name, str_ent, str_result);
		
		return result;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Util:Entity_Overlays")
		{
			MOD_ADD_DETOUR_STATIC(DrawAllDebugOverlays, "DrawAllDebugOverlays");
			
			MOD_ADD_DETOUR_STATIC(SetDebugBits, "SetDebugBits");
			
			MOD_ADD_DETOUR_STATIC(GetNextCommandEntity, "GetNextCommandEntity");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_util_entity_overlays", "0", FCVAR_NOTIFY,
		"Utility: enable entity overlays regardless of developer convar or dedicated server status",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
