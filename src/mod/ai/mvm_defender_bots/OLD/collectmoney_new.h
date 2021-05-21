#ifndef _INCLUDE_SIGSEGV_MOD_AI_MVM_DEFENDER_BOTS_ACTIONS_COLLECTMONEY_H_
#define _INCLUDE_SIGSEGV_MOD_AI_MVM_DEFENDER_BOTS_ACTIONS_COLLECTMONEY_H_


#include "re/nextbot.h"
#include "re/path.h"
#include "stub/tfbot.h"
#include "stub/entities.h"


namespace Mod::AI::MvM_Defender_Bots
{
	/* collect a currency pack */
	class CTFBotMvMCollectMoney : public IHotplugAction<CTFBot>
	{
	public:
		CTFBotMvMCollectMoney() {}
		
		virtual const char *GetName() const override { return "MvMCollectMoney"; }
		
		
		virtual QueryResponse ShouldHurry(const INextBot *nextbot) const override
		{
			return QueryResponse::YES;
		}
		
		virtual QueryResponse ShouldRetreat(const INextBot *nextbot) const override
		{
			return QueryResponse::NO;
		}
		
//		virtual QueryResponse ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override
//		{
//			return QueryResponse::DONTCARE;
//		}
		
//		virtual QueryResponse IsHindrance(const INextBot *nextbot, CBaseEntity *it) const override
//		{
//			return QueryResponse::DONTCARE;
//		}
		
//		virtual Vector SelectTargetPoint(const INextBot *nextbot, const CBaseCombatCharacter *them) const override
//		{
//			return vec3_origin;
//		}
		
//		virtual QueryResponse IsPositionAllowed(const INextBot *nextbot, const Vector& v1) const override
//		{
//			return QueryResponse::DONTCARE;
//		}
		
//		virtual const CKnownEntity *SelectMoreDangerousThreat(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const override
//		{
//			return nullptr;
//		}
		
		
		virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override
		{
			
		}
		
		virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override
		{
			
		}
		
		virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override
		{
			
		}
		
		
//		virtual ActionResult<CTFBot> OnSuspend(CTFBot *actor, Action<CTFBot> *action) override
//		{
//			
//		}
		
//		virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override
//		{
//			
//		}
		
		
//		virtual EventDesiredResult<CTFBot> OnMoveToSuccess(CTFBot *actor, const Path *path) override
//		{
//			
//		}
		
//		virtual EventDesiredResult<CTFBot> OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail) override
//		{
//			
//		}
		
		virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override
		{
			return EventDesiredResult<CTFBot>::Done("Stuck! Giving up.");
		}
		
		static bool IsPossible(CTFBot *actor);
		
	private:
		
	};
}


#endif
