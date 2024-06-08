#include "mod.h"
#include "mem/scan.h"


namespace Mod::Etc::Suppress_NB_Event_Messages
{
	const char *p_fmtstr = nullptr;
	
	
	DETOUR_DECL_STATIC_CALL_CONVENTION(__gcc_regcall, SpewRetval_t, D__SpewMessage, SpewType_t spewType, const char *pGroupName, int nLevel, const Color *pColor, const char *pMsgFormat, va_list args)
	{
		/* intentional pointer comparison */
		if (pMsgFormat == p_fmtstr) {
			return SPEW_CONTINUE;
		}
		
		return DETOUR_STATIC_CALL(spewType, pGroupName, nLevel, pColor, pMsgFormat, args);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Suppress_NB_Event_Messages")
		{
			MOD_ADD_DETOUR_STATIC(D__SpewMessage, "_SpewMessage [internal]");
		}
		
		virtual bool OnLoad() override
		{
			p_fmtstr = Scan::FindUniqueConstStr(Library::SERVER, "%3.2f: WARNING: %s::%s() RESULT_CRITICAL collision\n");
			if (p_fmtstr == nullptr) return false;
			
			return true;
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_etc_suppress_nb_event_messages", "0", FCVAR_NOTIFY,
		"Mod: suppress NextBot event RESULT_CRITICAL collision console messages",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
