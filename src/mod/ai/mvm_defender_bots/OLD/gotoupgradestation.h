#ifndef _INCLUDE_SIGSEGV_MOD_AI_MVM_DEFENDER_BOTS_GOTOUPGRADESTATION_H_
#define _INCLUDE_SIGSEGV_MOD_AI_MVM_DEFENDER_BOTS_GOTOUPGRADESTATION_H_


#include "re/nextbot.h"
#include "re/path.h"
#include "stub/tfbot.h"
#include "stub/entities.h"


namespace Mod::AI::MvM_Defender_Bots
{
	/* find a func_upgradestation and path toward it */
	class CTFBotGoToUpgradeStation : public IHotplugAction<CTFBot>
	{
	public:
		CTFBotGoToUpgradeStation() {}
		
		virtual const char *GetName() const override { return "GoToUpgradeStation"; }
		
		virtual QueryResponse ShouldHurry(const INextBot *nextbot) const override { return QueryResponse::YES; }
		
		virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
		virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
		virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
		
		virtual EventDesiredResult<CTFBot> OnMoveToSuccess(CTFBot *actor, const Path *path) override;
		virtual EventDesiredResult<CTFBot> OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail) override;
		virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
		
	private:
		bool FindClosestUpgradeStation();
		
		CHandle<CUpgrades> m_hUpgradeStation;
		Vector m_vecUpgradeStation;
		
		PathFollower m_PathFollower;
		CountdownTimer m_ctRecomputePath;
	};
}


#endif
