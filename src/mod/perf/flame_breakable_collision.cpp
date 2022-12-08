#include "mod.h"
#include "util/scope.h"


namespace Mod::Perf::Flame_Breakable_Collision
{
	RefCount rc_CTFFlameEntity_FlameThink;
	DETOUR_DECL_MEMBER(void, CTFFlameEntity_FlameThink)
	{
		SCOPED_INCREMENT(rc_CTFFlameEntity_FlameThink);
		DETOUR_MEMBER_CALL(CTFFlameEntity_FlameThink)();
	}
	
	DETOUR_DECL_MEMBER(CBaseEntity *, CGlobalEntityList_FindEntityByClassname, CBaseEntity *pStartEntity, const char *szName, IEntityFindFilter *filter)
	{
		if (rc_CTFFlameEntity_FlameThink > 0 && strcmp(szName, "func_breakable") == 0) {
			return nullptr;
		}
		
		return DETOUR_MEMBER_CALL(CGlobalEntityList_FindEntityByClassname)(pStartEntity, szName, filter);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Perf:Flame_Breakable_Collision")
		{
			MOD_ADD_DETOUR_MEMBER(CTFFlameEntity_FlameThink,               "CTFFlameEntity::FlameThink");
			MOD_ADD_DETOUR_MEMBER(CGlobalEntityList_FindEntityByClassname, "CGlobalEntityList::FindEntityByClassname");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_perf_flame_breakable_collision", "0", FCVAR_NOTIFY,
		"Mod: improve MvM flame entity performance by eliding func_breakable classname lookups",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
