/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/scenario/capture_the_flag/tf_bot_escort_flag_carrier.cpp
 * used in MvM: TODO
 */


ConVar tf_bot_flag_escort_give_up_range("tf_bot_flag_escort_give_up_range", "1000", FCVAR_CHEAT);
ConVar tf_bot_flag_escort_max_count("tf_bot_flag_escort_max_count", "4", FCVAR_CHEAT);


CTFBotEscortFlagCarrier()
{
}

~CTFBotEscortFlagCarrier()
{
}


const char *GetName() const
{
	return "EscortFlagCarrier";
}


ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> Update(CTFBot *actor, float dt)
{
	CCaptureFlag *flag = actor->GetFlagToFetch();
	if (flag == nullptr) {
		return ActionResult<CTFBot>::Done("No flag");
	}
	
	CTFPlayer *carrier = ToTFPlayer(flag->GetOwnerEntity());
	if (carrier == nullptr) {
		return ActionResult<CTFBot>::Done("Flag was dropped");
	}
	
	if (actor->IsSelf(carrier)) {
		return ActionResult<CTFBot>::Done("I picked up the flag!");
	}
	
	if (actor->IsRangeGreaterThan(carrier, tf_bot_flag_escort_give_up_range.GetFloat()) &&
		actor->SelectRandomReachableEnemy() != nullptr) {
		return ActionResult<CTFBot>::ChangeTo(new CTFBotAttackFlagDefenders(),
			"Too far from flag carrier - attack defenders!");
	}
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
	if (threat != nullptr && threat->IsVisibleRecently()) {
		actor->EquipBestWeaponForThreat(threat);
	}
	
	CTFWeaponBase *weapon = actor->GetActiveTFWeapon();
	if (weapon != nullptr && weapon->IsMeleeWeapon() &&
		actor->IsRangeLessThan(carrier, tf_bot_flag_escort_range.GetFloat()) &&
		actor->IsLineOfSightClear(carrier, IGNORE_NOTHING)) {
		ActionResult<CTFBot> result = this->m_MeleeAttack.Update(actor, dt);
		
		if (result.transition == ActionTransition::CONTINUE ||
			!actor->IsRangeGreaterThan(carrier, 0.5f * tf_bot_flag_escort_range.GetFloat())) {
			return ActionResult<CTFBot>::Continue();
		}
	}
	
	if (this->m_ctRecomputePath.IsElapsed()) {
		if (GetBotEscortCount(actor->GetTeamNumber()) > tf_bot_flag_escort_max_count.GetInt() &&
			actor->SelectRandomReachableEnemy() != nullptr) {
			return ActionResult<CTFBot>::Done("Too many flag escorts - giving up");
		}
		
		CTFBotPathCost cost_func(actor, FASTEST_ROUTE);
		this->m_PathFollower.Compute(actor, carrier, cost_func, 0.0f, true);
		
		this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
	}
	
	this->m_PathFollower.Update(actor);
	
	return ActionResult<CTFBot>::Continue();
}


int GetBotEscortCount(int teamnum)
{
	CUtlVector<CTFPlayer *> teammates;
	CollectPlayers(&teammates, teamnum, true, false);
	
	int count = 0;
	FOR_EACH_VEC(teammates, i) {
		CTFBot *bot = dynamic_cast<CTFBot *>(teammates[i]);
		if (bot == nullptr) continue;
		
		Behavior<CTFBot> *behavior = bot->GetIntentionInterface()->FirstContainedResponder();
		if (behavior == nullptr) continue;
		
		auto action = static_cast<Action<CTFBot> *>(behavior->FirstContainedResponder());
		if (action == nullptr) continue;
		
		while (action->FirstContainedResponder() != nullptr) {
			action = static_cast<Action<CTFBot> *>(action->FirstContainedResponder());
		}
		
		if (action->IsNamed("EscortFlagCarrier")) {
			++count;
		}
	}
	
	return count;
}
