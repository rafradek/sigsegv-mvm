#include "mod.h"


namespace Mod::Visualize::Huntsman
{
	ConVar cvar_interval("sig_visualize_huntsman_interval", "2", FCVAR_NOTIFY,
		"Visualization: frame interval");
	
	
	
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Visualize:Huntsman") {}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			static long frame = 0;
			if (++frame % cvar_interval.GetInt() != 0) return;
			
			// TODO
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_visualize_huntsman", "0", FCVAR_NOTIFY,
		"Visualization: draw huntsman hit detection information",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
