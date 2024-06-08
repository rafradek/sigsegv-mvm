#include "mod.h"
#include "stub/tfplayer.h"


namespace Mod::Debug::Vaccinator
{
	#warning __gcc_regcall detours considered harmful!
	DETOUR_DECL_STATIC_CALL_CONVENTION(__gcc_regcall, bool, CheckMedicResist, ETFCond iCondUber, ETFCond iCondPassive, CTFPlayer *pPlayer, float& f1, bool bCrit, float& f2)
	{
		// TODO
		return DETOUR_STATIC_CALL(iCondUber, iCondPassive, pPlayer, f1, bCrit, f2);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:Vaccinator")
		{
			MOD_ADD_DETOUR_STATIC(CheckMedicResist, "CheckMedicResist");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_vaccinator", "0", FCVAR_NOTIFY,
		"Debug: print the inputs and outputs of the CheckMedicResist function",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
