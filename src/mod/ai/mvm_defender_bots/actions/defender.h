#ifndef _INCLUDE_SIGSEGV_MOD_AI_MVM_DEFENDER_BOTS_ACTIONS_DEFENDER_H_
#define _INCLUDE_SIGSEGV_MOD_AI_MVM_DEFENDER_BOTS_ACTIONS_DEFENDER_H_


#include "re/nextbot.h"
#include "stub/tfbot.h"


namespace Mod::AI::MvM_Defender_Bots
{
	/* main parent action for MvM defender bots */
	class CTFBotMvMDefender : public IHotplugAction<CTFBot>
	{
	public:
		CTFBotMvMDefender() {}
		
		virtual const char *GetName() const override { return "MvMDefender"; }
		
		virtual QueryResponse ShouldHurry(const INextBot *nextbot) const override;
		virtual QueryResponse ShouldRetreat(const INextBot *nextbot) const override;
		virtual QueryResponse ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
//		virtual QueryResponse IsHindrance(const INextBot *nextbot, CBaseEntity *it) const override;
//		virtual Vector SelectTargetPoint(const INextBot *nextbot, const CBaseCombatCharacter *them) const override;
//		virtual QueryResponse IsPositionAllowed(const INextBot *nextbot, const Vector& v1) const override;
//		virtual const CKnownEntity *SelectMoreDangerousThreat(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const override;
		
		virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
		virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
		virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
		
//		virtual ActionResult<CTFBot> OnSuspend(CTFBot *actor, Action<CTFBot> *action) override;
//		virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
		
		virtual Action<CTFBot> *InitialContainedAction(CTFBot *actor) override;
		
	private:
		
	};
}


#endif
