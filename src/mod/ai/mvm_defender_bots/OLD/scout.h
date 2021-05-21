#ifndef _INCLUDE_SIGSEGV_MOD_AI_MVM_DEFENDER_BOTS_SCOUT_H_
#define _INCLUDE_SIGSEGV_MOD_AI_MVM_DEFENDER_BOTS_SCOUT_H_


#include "re/nextbot.h"
#include "re/path.h"
#include "stub/tfbot.h"


namespace Mod::AI::MvM_Defender_Bots
{
	/* run around and do scout stuff */
	class CTFBotMvMDefenderScout : public IHotplugAction<CTFBot>
	{
	public:
		CTFBotMvMDefenderScout() {}
		
		virtual const char *GetName() const override { return "MvMDefenderScout"; }
		
		virtual QueryResponse ShouldHurry(const INextBot *nextbot) const override { return QueryResponse::YES; }
		
		virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
		virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
		virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
		
		virtual Action<CTFBot> *InitialContainedAction(CTFBot *actor) override;
		
	private:
	};
	
	
	/* mark giants for death */
	class CTFBotMarkGiants : public IHotplugAction<CTFBot>
	{
	public:
		CTFBotMarkGiants() {}
		
		virtual const char *GetName() const override { return "MarkGiants"; }
		
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
