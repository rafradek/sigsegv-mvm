#include "mod.h"
#include "stub/server.h"
//#include "particles/particles.h"

namespace Mod::Etc::Particle_Stringtable_Fix
{
	class CParticleSystemMgr {};

	MemberFuncThunk<CParticleSystemMgr *, int> ft_GetParticleSystemCount("CParticleSystemMgr::GetParticleSystemCount");
	MemberFuncThunk<CParticleSystemMgr *, void> ft_CParticleSystemMgr_C2("CParticleSystemMgr::CParticleSystemMgr");
	MemberFuncThunk<CParticleSystemMgr *, void> ft_CParticleSystemMgr_D2("CParticleSystemMgr::~CParticleSystemMgr");
	MemberFuncThunk<CParticleSystemMgr *, void, void *> ft_CParticleSystemMgr_Init("CParticleSystemMgr::Init");
	StaticFuncThunk<void, bool, bool> ft_ParseParticleEffects("ParseParticleEffects");

	GlobalThunk<CParticleSystemMgr> g_pParticleSystemMgr("g_pParticleSystemMgr");
	GlobalThunk<CParticleSystemMgr> g_pParticleSystemQuery("g_pParticleSystemQuery");

    DETOUR_DECL_MEMBER(INetworkStringTable *, CNetworkStringTableContainer_CreateStringTable, const char *tableName, int maxentries, int userdatafixedsize, int userdatanetworkbits)
	{
		if (strcmp(tableName, "ParticleEffectNames") == 0) {
			maxentries *= 2;
			Msg("Now is %d\n",maxentries );
		}
        return DETOUR_MEMBER_CALL(CNetworkStringTableContainer_CreateStringTable)(tableName, maxentries, userdatafixedsize, userdatanetworkbits);
	}
    DETOUR_DECL_MEMBER(bool, CServerGameDLL_LevelInit, const char *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background )
	{
		ft_CParticleSystemMgr_D2(&g_pParticleSystemMgr.GetRef());
		ft_CParticleSystemMgr_C2(&g_pParticleSystemMgr.GetRef());
		ft_CParticleSystemMgr_Init(&g_pParticleSystemMgr.GetRef(), &g_pParticleSystemQuery.GetRef());
		ft_ParseParticleEffects(false, false);
        return DETOUR_MEMBER_CALL(CServerGameDLL_LevelInit)(pMapName, pMapEntities, pOldLevel, pLandmarkName, loadGame, background);
	}

    class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Particle_Stringtable_Fix")
		{
			//MOD_ADD_DETOUR_MEMBER(CNetworkStringTableContainer_CreateStringTable, "CNetworkStringTableContainer::CreateStringTable");
			//MOD_ADD_DETOUR_MEMBER(CNetworkStringTable_AddString, "CNetworkStringTable::AddString");
			MOD_ADD_DETOUR_MEMBER(CServerGameDLL_LevelInit, "CServerGameDLL::LevelInit");
		}
	};
	CMod s_Mod;

    ConVar cvar_enable("sig_etc_particle_precache_fix", "0", FCVAR_NOTIFY,
		"Mod: Fix particle errors due to particle stringtable overflow",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}