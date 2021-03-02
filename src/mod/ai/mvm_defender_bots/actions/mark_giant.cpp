#include "mod.h"
#include "mod/ai/mvm_defender_bots/helpers.h"
#include "mod/ai/mvm_defender_bots/actions/mark_giant.h"
#include "stub/tfbot_behavior.h"


namespace Mod::AI::MvM_Defender_Bots
{
	CTFWeaponBase *GetMarkForDeathWeapon(CTFPlayer *player)
	{
		for (int i = 0; i < MAX_WEAPONS; ++i) {
			CBaseCombatWeapon *cbcw = player->Weapon_GetSlot(i);
			if (cbcw == nullptr) continue;
			
			auto weapon = rtti_cast<CTFWeaponBase *>(cbcw);
			if (weapon == nullptr) continue;
			
			int attr = CAttributeManager::AttribHookValue<int>(0, "mark_for_death", weapon);
			if (attr == 0) continue;
			
			return weapon;
		}
		
		return nullptr;
	}
	
	
	bool IsPlayerMarkable(CTFBot *bot, CTFPlayer *victim)
	{
		/* must be alive */
		if (!victim->IsAlive()) return false;
		
		/* must be an enemy */
		if (bot->GetTeamNumber() == victim->GetTeamNumber()) return false;
		
		/* must be a giant */
		if (!victim->IsMiniBoss()) return false;
		
		/* must not already be marked for death */
		if (victim->m_Shared->InCond(TF_COND_MARKEDFORDEATH)) return false;
		
		/* must not be invulnerable */
		if (victim->m_Shared->IsInvulnerable()) return false;
		
		/* must not be a sentry buster */
		CTFBot *vbot = ToTFBot(victim);
		if (vbot != nullptr && vbot->GetMission() == CTFBot::MISSION_DESTROY_SENTRIES) return false;
		
		return true;
	}
	
	
	QueryResponse CTFBotMarkGiant::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
	{
		CTFPlayer *player = ToTFPlayer(threat->GetEntity());
		if (player != nullptr && player->IsMiniBoss()) {
			return QueryResponse::YES;
		}
		
		return QueryResponse::NO;
	}
	
	
	ActionResult<CTFBot> CTFBotMarkGiant::OnStart(CTFBot *actor, Action<CTFBot> *action)
	{
		this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
		
		CTFWeaponBase *weapon = GetMarkForDeathWeapon(actor);
		if (weapon == nullptr) {
			return ActionResult<CTFBot>::Done("Don't have a mark-for-death weapon.");
		}
		
		std::vector<CTFPlayer *> potential_victims;
		ForEachPlayer([&](CBasePlayer *player, bool& done){
			CTFPlayer *tfplayer = ToTFPlayer(player);
			if (tfplayer == nullptr) return;
			
			if (IsPlayerMarkable(actor, tfplayer)) {
				potential_victims.push_back(tfplayer);
			}
		});
		
		if (potential_victims.empty()) {
			return ActionResult<CTFBot>::Done("No eligible mark victims.");
		}
		
		this->m_hTarget = potential_victims[RandomInt(0, potential_victims.size() - 1)];
		
		actor->PushRequiredWeapon(weapon);
		
		return ActionResult<CTFBot>::Continue();
	}
	
	ActionResult<CTFBot> CTFBotMarkGiant::Update(CTFBot *actor, float dt)
	{
		if (this->m_hTarget == nullptr || !this->m_hTarget->IsAlive()) {
			return ActionResult<CTFBot>::Done("Mark target is no longer valid.");
		}
		
		if (!IsPlayerMarkable(actor, this->m_hTarget)) {
			return ActionResult<CTFBot>::Done("Mark target is no longer markable.");
		}
		
		auto nextbot = actor->MyNextBotPointer();
		
		if (this->m_ctRecomputePath.IsElapsed()) {
			this->m_ctRecomputePath.Start(RandomFloat(0.4f, 0.6f));
			
			CTFBotPathCost cost_func(actor, FASTEST_ROUTE);
			this->m_PathFollower.Compute(nextbot, this->m_hTarget->GetAbsOrigin(), cost_func, 0.0f, true);
			
			// TODO: handle Path::Compute failure
		}
		
		float dist_to_target = (actor->GetAbsOrigin() - this->m_hTarget->GetAbsOrigin()).Length();
		if (dist_to_target < 512.0f) {
			actor->GetBodyInterface()->AimHeadTowards(this->m_hTarget->WorldSpaceCenter(),
				IBody::LookAtPriorityType::CRITICAL, 0.1f, nullptr, "Look at our mark-for-death target");
		}
		
		if (dist_to_target > actor->GetDesiredAttackRange()) {
			this->m_PathFollower.Update(nextbot);
		}
		
		return ActionResult<CTFBot>::Continue();
	}
	
	void CTFBotMarkGiant::OnEnd(CTFBot *actor, Action<CTFBot> *action)
	{
		actor->PopRequiredWeapon();
	}
	
	
	EventDesiredResult<CTFBot> CTFBotMarkGiant::OnMoveToSuccess(CTFBot *actor, const Path *path)
	{
		DevMsg("CTFBotMarkGiant::OnMoveToSuccess(#%d)\n", ENTINDEX(actor));
		return EventDesiredResult<CTFBot>::Continue();
	}
	
	EventDesiredResult<CTFBot> CTFBotMarkGiant::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
	{
		return EventDesiredResult<CTFBot>::Done("MoveTo failed! Giving up.");
	}
	
	EventDesiredResult<CTFBot> CTFBotMarkGiant::OnStuck(CTFBot *actor)
	{
		return EventDesiredResult<CTFBot>::Done("Stuck! Giving up.");
	}
	
	
	bool CTFBotMarkGiant::IsPossible(CTFBot *actor)
	{
		if (GetMarkForDeathWeapon(actor) == nullptr) return false;
		
		bool victim_exists = false;
		
		ForEachPlayer([&](CBasePlayer *player, bool& done){
			CTFPlayer *tfplayer = ToTFPlayer(player);
			if (tfplayer == nullptr) return;
			
			if (IsPlayerMarkable(actor, tfplayer)) {
				victim_exists = true;
				done = true;
			}
		});
		
		return victim_exists;
	}
}
