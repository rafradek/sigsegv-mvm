#ifndef _INCLUDE_SIGSEGV_MOD_AI_MVM_DEFENDER_BOTS_ACTIONS_PREWAVE_H_
#define _INCLUDE_SIGSEGV_MOD_AI_MVM_DEFENDER_BOTS_ACTIONS_PREWAVE_H_


#include "re/nextbot.h"
#include "stub/tfbot.h"


namespace Mod::AI::MvM_Defender_Bots
{
	/* do pre-wave stuff */
	class CTFBotPreWave : public IHotplugAction<CTFBot>
	{
	public:
		CTFBotPreWave() {}
		
		virtual const char *GetName() const override { return "PreWave"; }
		
		virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
		virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
		virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
		
	private:
		bool IsReady(CTFBot *actor);
	};
}


#endif
