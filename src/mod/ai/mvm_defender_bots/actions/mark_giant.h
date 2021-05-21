#ifndef _INCLUDE_SIGSEGV_MOD_AI_MVM_DEFENDER_BOTS_ACTIONS_MARK_GIANT_H_
#define _INCLUDE_SIGSEGV_MOD_AI_MVM_DEFENDER_BOTS_ACTIONS_MARK_GIANT_H_


#include "re/nextbot.h"
#include "re/path.h"
#include "stub/tfbot.h"
#include "stub/entities.h"


namespace Mod::AI::MvM_Defender_Bots
{
	/* mark a giant for death */
	class CTFBotMarkGiant : public IHotplugAction<CTFBot>
	{
	public:
		CTFBotMarkGiant() {}
		
		virtual const char *GetName() const override { return "MarkGiant"; }
		
		virtual QueryResponse ShouldHurry(const INextBot *nextbot) const override   { return QueryResponse::YES; }
		virtual QueryResponse ShouldRetreat(const INextBot *nextbot) const override { return QueryResponse::NO; }
		virtual QueryResponse ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
		
		virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
		virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
		virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
		
		virtual EventDesiredResult<CTFBot> OnMoveToSuccess(CTFBot *actor, const Path *path) override;
		virtual EventDesiredResult<CTFBot> OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail) override;
		virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
		
		static bool IsPossible(CTFBot *actor);
		
	private:
		CHandle<CTFPlayer> m_hTarget;
		
		PathFollower m_PathFollower;
		CountdownTimer m_ctRecomputePath;
	};
}


#endif
