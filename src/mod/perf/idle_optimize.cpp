#include "mod.h"
#include "stub/server.h"

namespace Mod::Perf::Idle_Optimize
{   

    bool hibernated = false;
    DETOUR_DECL_MEMBER(void, CGameServer_SetHibernating, bool hibernate)
	{
		hibernated = hibernate;
		DETOUR_MEMBER_CALL(hibernate);
	}
    extern ConVar cvar_enable;

    DETOUR_DECL_STATIC(void, SV_Frame, bool finaltick)
	{
        if (!hibernated) {
            DETOUR_STATIC_CALL(finaltick);
            return;
        }
        
        static int accum_ticks = 0;
        int batchCount = cvar_enable.GetInt();
        if (accum_ticks++ % batchCount == 0) {
            for (int i = 0; i < batchCount; i++) {
		        DETOUR_STATIC_CALL(i == batchCount - 1);
            }
        }
        else {
            ((CBaseServer *)sv)->RunFrame();
        }
	}

    class CMod : public IMod
	{
	public:
		CMod() : IMod("Perf::Idle_Optimize")
		{
			MOD_ADD_DETOUR_MEMBER(CGameServer_SetHibernating, "CGameServer::SetHibernating");
            MOD_ADD_DETOUR_STATIC(SV_Frame, "SV_Frame");
        }
    };
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_perf_idle_optimize", "0", FCVAR_NOTIFY,
		"Mod: Batch this many frames to update at once when the server is idle. 0 to disable",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}