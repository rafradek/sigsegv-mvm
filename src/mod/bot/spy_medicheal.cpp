#include "mod.h"
#include "stub/tfbot.h"
#include "stub/tf_shareddefs.h"
#include "util/iterate.h"


namespace Mod::Bot::Spy_MedicHeal
{
	
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Bot:Spy_MedicHeal")
		{
			
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			static long frame = 0;
			if (++frame % 2 == 0) return;
			
		//	ForEachTFPlayer([](CTFPlayer *player){
		//		
		//	});
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_bot_spy_medicheal", "0", FCVAR_NOTIFY,
		"Mod: debug/fix bad spy logic in CTFBotMedicHeal",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
