#include "mod.h"


namespace Mod::Prof::Flame_Breakable_Collision
{
	DETOUR_DECL_MEMBER(void, CTFFlameEntity_FlameThink)
	{
		VPROF_BUDGET("CTFFlameEntity::FlameThink", "sigsegv");
		DETOUR_MEMBER_CALL(CTFFlameEntity_FlameThink)();
	}
	
	DETOUR_DECL_MEMBER(CBaseEntity *, CGlobalEntityList_FindEntityByClassname, CBaseEntity *pStartEntity, const char *szName, IEntityFindFilter *filter)
	{
		VPROF_BUDGET("CGlobalEntityList::FindEntityByClassname", "sigsegv");
		return DETOUR_MEMBER_CALL(CGlobalEntityList_FindEntityByClassname)(pStartEntity, szName, filter);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Prof:Flame_Breakable_Collision")
		{
			MOD_ADD_DETOUR_MEMBER(CTFFlameEntity_FlameThink,               "CTFFlameEntity::FlameThink");
			MOD_ADD_DETOUR_MEMBER(CGlobalEntityList_FindEntityByClassname, "CGlobalEntityList::FindEntityByClassname");
			
		//	this->AddDetour(new CFuncVProf(Library::SERVER, "CTFFlameEntity.*FlameThink",               "CTFFlameEntity::FlameThink",               "sigsegv"));
		//	this->AddDetour(new CFuncVProf(Library::SERVER, "CGlobalEntityList21FindEntityByClassname", "CGlobalEntityList::FindEntityByClassname", "sigsegv"));
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_prof_flame_breakable_collision", "0", FCVAR_NOTIFY,
		"Mod: profile flame entity think performance",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
