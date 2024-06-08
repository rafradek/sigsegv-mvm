#include "mod.h"


namespace Mod::Util::DTWarning_Use_CVar
{
	DETOUR_DECL_STATIC(void, DataTable_Warning, const char *pInMessage, ...)
	{
		static ConVarRef dtwarning("dtwarning");
		if (dtwarning.IsValid() && dtwarning.GetInt() == 0) return;
		
		// TODO: variadic forwarding that doesn't involve extra copying...
		char msg[4096];
		
		va_list va;
		va_start(va, pInMessage);
		V_vsprintf_safe(msg, pInMessage, va);
		va_end(va);
		
		DETOUR_STATIC_CALL("%s", msg);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Util:DTWarning_Use_CVar")
		{
			MOD_ADD_DETOUR_STATIC(DataTable_Warning, "DataTable_Warning");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_util_dtwarning_use_cvar", "0", FCVAR_NOTIFY,
		"Utility: make the dtwarning engine cvar actually control whether DataTable_Warning prints messages",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
