#include "mod.h"
#include "util/scope.h"


namespace Mod::Util::Frame_Prof
{
    CCycleCount timespent;
    float prevTime = 0.0f;
	DETOUR_DECL_STATIC(void, _Host_RunFrame, float dt)
	{
        CTimeAdder timer(&timespent);
		DETOUR_STATIC_CALL(_Host_RunFrame)(dt);
        timer.End();
        if (floor(gpGlobals->curtime/5) != floor(prevTime) ) {
            DevMsg("Frame time: %f\n", timespent.GetSeconds()/5);
            timespent.Init();
            prevTime = gpGlobals->curtime/5;
        }
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Util:Frame_Prof")
		{
			MOD_ADD_DETOUR_STATIC(_Host_RunFrame,               "_Host_RunFrame");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_util_frame_prof", "0", FCVAR_NOTIFY,
		"Mod: show frame time %",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}