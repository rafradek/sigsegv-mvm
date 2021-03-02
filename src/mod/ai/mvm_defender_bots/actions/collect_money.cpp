#include "mod.h"
#include "mod/ai/mvm_defender_bots/actions/collect_money.h"
#include "mod/ai/mvm_defender_bots/helpers.h"
#include "mod/ai/mvm_defender_bots/trackers.h"
#include "stub/gamerules.h"
#include "util/scope.h"


namespace Mod::AI::MvM_Defender_Bots
{
	// currency magnet range: (absorigin to absorigin)
	// scout: 288 HU
	// other: 72 HU
	
	
	float GetTimeUntilRemoved(CTFPowerup *powerup)
	{
		return (powerup->GetNextThink("PowerupRemoveThink") - gpGlobals->curtime);
	}
	
	
	bool CompareCurrencyPacks(CCurrencyPack *lhs, CCurrencyPack *rhs)
	{
		float t_remove_lhs = GetTimeUntilRemoved(lhs);
		float t_remove_rhs = GetTimeUntilRemoved(rhs);
		
		bool is_red_lhs = lhs->IsDistributed();
		bool is_red_rhs = rhs->IsDistributed();
		
		int value_lhs = lhs->GetAmount();
		int value_rhs = rhs->GetAmount();
		
		//float dist_lhs = 
	}
	
	
	// TODO: prioritize currency packs based on these criteria:
	// - age (0~30 seconds)
	// - type (green vs red)
	// - credit value
	// - distance
	
	
	ActionResult<CTFBot> CTFBotCollectMoney::OnStart(CTFBot *actor, Action<CTFBot> *action)
	{
		this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
		
		this->SelectCurrencyPack();
		
		return ActionResult<CTFBot>::Continue();
	}
	
	ActionResult<CTFBot> CTFBotCollectMoney::Update(CTFBot *actor, float dt)
	{
		if (this->m_hCurrencyPack == nullptr) {
			return ActionResult<CTFBot>::Done("Currency pack is gone.");
		}
		
		auto nextbot = actor->MyNextBotPointer();
		
		if (this->m_ctRecomputePath.IsElapsed()) {
			this->m_ctRecomputePath.Start(RandomFloat(0.4f, 0.6f));
			
			CTFBotPathCost cost_func(actor, FASTEST_ROUTE);
			this->m_PathFollower.Compute(nextbot, this->m_hCurrencyPack->GetAbsOrigin(), cost_func, 0.0f, true);
			
			// TODO: handle Path::Compute failure
		}
		
		this->m_PathFollower.Update(nextbot);
		
		return ActionResult<CTFBot>::Continue();
	}
	
	void CTFBotCollectMoney::OnEnd(CTFBot *actor, Action<CTFBot> *action)
	{
		
	}
	
	
//	EventDesiredResult<CTFBot> CTFBotCollectMoney::OnMoveToSuccess(CTFBot *actor, const Path *path)
//	{
//		DevMsg("CTFBotCollectMoney::OnMoveToSuccess(#%d)\n", ENTINDEX(actor));
//		return EventDesiredResult<CTFBot>::Continue();
//	}
	
//	EventDesiredResult<CTFBot> CTFBotCollectMoney::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
//	{
//		DevMsg("CTFBotCollectMoney::OnMoveToFailure(#%d, %d)\n", ENTINDEX(actor), (int)fail);
//		return EventDesiredResult<CTFBot>::Continue();
//	}
	
	EventDesiredResult<CTFBot> CTFBotCollectMoney::OnStuck(CTFBot *actor)
	{
		return EventDesiredResult<CTFBot>::Done("Stuck! Giving up.");
	}
	
	
	bool CTFBotCollectMoney::IsPossible(CTFBot *actor)
	{
		if (!TFGameRules()->IsMannVsMachineMode()) return false;
		if (!actor->IsPlayerClass(TF_CLASS_SCOUT)) return false;
		
		if (TheCreditTracker.SelectCurrencyPack() == nullptr) return false;
		
		return true;
	}
	
	
	void CTFBotCollectMoney::SelectCurrencyPack()
	{
		this->m_hCurrencyPack = TheCreditTracker.SelectCurrencyPack();
	}
}
