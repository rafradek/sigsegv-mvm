#include "mod.h"


namespace Mod::Util::Console_Send
{
	SpewOutputFunc_t LocalSpewOutputFunc = nullptr;
	
	
	SpewRetval_t SpewRedirectFunc(SpewType_t type, const char *pMsg)
	{
		static thread_local bool reentrancy_guard = false;
		
		if (!reentrancy_guard) {
			reentrancy_guard = true;
			
			Color c = *GetSpewOutputColor();
			
			IGameEvent *event = gameeventmanager->CreateEvent("console_forward");
			if (event != nullptr) {
				event->SetInt("type", type);
				
				event->SetInt("r", c.r());
				event->SetInt("g", c.g());
				event->SetInt("b", c.b());
				
				event->SetString("str", pMsg);
				
				gameeventmanager->FireEvent(event);
			}/* else {
				fprintf(stderr, "DEBUG: gameeventmanager->CreateEvent failed\n");
			}*/
			
			reentrancy_guard = false;
		}/* else {
			fprintf(stderr, "DEBUG: reentrancy_guard is held\n");
		}*/
		
		/*if (LocalSpewOutputFunc != nullptr) {
			return LocalSpewOutputFunc(type, pMsg);
		} else {
			return DefaultSpewFunc(type, pMsg);
		}*/
		return SPEW_ABORT;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Util:Console_Send") {}
		
		virtual void OnEnable() override
		{
			LocalSpewOutputFunc = GetSpewOutputFunc();
			SpewOutputFunc(&SpewRedirectFunc);
		}
		virtual void OnDisable() override
		{
			if (LocalSpewOutputFunc != nullptr) {
				SpewOutputFunc(LocalSpewOutputFunc);
				LocalSpewOutputFunc = nullptr;
			}
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_util_console_send", "0", FCVAR_NOTIFY,
		"Utility: console forwarding: server send",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
