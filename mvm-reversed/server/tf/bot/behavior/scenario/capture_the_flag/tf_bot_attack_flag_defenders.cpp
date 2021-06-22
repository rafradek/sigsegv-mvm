/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/scenario/capture_the_flag/tf_bot_attack_flag_defenders.cpp
 * used in MvM: TODO
 */


ConVar tf_bot_flag_escort_range("tf_bot_flag_escort_range", "500", FCVAR_CHEAT);


CTFBotAttackFlagDefenders::CTFBotAttackFlagDefenders(float duration)
{
	if (duration > 0.0f) {
		this->m_ctActionDuration.Start(duration);
	}
}

CTFBotAttackFlagDefenders::~CTFBotAttackFlagDefenders()
{
}


const char *CTFBotAttackFlagDefenders::GetName() const
{
	return "AttackFlagDefenders";
}


ActionResult<CTFBot> CTFBotAttackFlagDefenders::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
	
	this->m_hTarget = nullptr;
	
	return CTFBotAttack::OnStart(actor, action);
}

ActionResult<CTFBot> CTFBotAttackFlagDefenders::Update(CTFBot *actor, float dt)
{
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
	if (threat != nullptr && threat->IsVisibleRecently()) {
		actor->EquipBestWeaponForThreat(threat);
	}
	
	if (this->m_ctCheckFlag.IsElapsed() && this->m_ctActionDuration.IsElapsed()) {
		this->m_ctCheckFlag.Start(RandomFloat(1.0f, 3.0f));
		
		CCaptureFlag *flag = actor->GetFlagToFetch();
		if (flag == nullptr) {
			return ActionResult<CTFBot>::Done("No flag");
		}
		
		if (!(TFGameRules()->IsMannVsMachineMode() && flag->IsHome())) {
			CTFPlayer *carrier = ToTFPlayer(flag->GetOwnerEntity());
			if (carrier == nullptr) {
				return ActionResult<CTFBot>::Done("Flag was dropped");
			}
			
			if (actor->IsSelf(carrier)) {
				return ActionResult<CTFBot>::Done("I picked up the flag!");
			}
			
			if (!(ToTFBot(carrier) != nullptr && carrier->m_Squad != nullptr)) {
				if (actor->IsRangeLessThan(carrier, tf_bot_flag_escort_range.GetFloat()) &&
					GetBotEscortCount(actor->GetTeamNumber()) < tf_bot_flag_escort_max_count.GetInt()) {
					return ActionResult<CTFBot>::ChangeTo(new CTFBotEscortFlagCarrier(),
						"Near flag carrier - escorting");
				}
			}
		}
	}
	
	ActionResult<CTFBot> result = CTFBotAttack::Update(actor, dt);
	if (result.transition != ActionTransition::DONE) {
		return ActionResult<CTFBot>::Continue();
	}
	
	if (this->m_hTarget == nullptr || !this->m_hTarget->IsAlive()) {
		CTFPlayer *target = actor->SelectRandomReachableEnemy();
		if (target == nullptr) {
			return ActionResult<CTFBot>::ChangeTo(new CTFBotEscortFlagCarrier(),
				"No reachable victim - escorting flag");
		}
		
		this->m_hTarget = target;
	}
	
	actor->GetVisionInterface()->AddKnownEntity(this->m_hTarget);
	
	if (this->m_ctRecomputePath.IsElapsed()) {
		this->m_ctRecomputePath.Start(RandomFloat(1.0f, 3.0f));
		
		CTFBotPathCost cost_func(actor, DEFAULT_ROUTE);
		this->m_PathFollower.Compute(actor, this->m_hTarget, cost_func, 0.0f, true);
	}
	
	this->m_PathFollower.Update(actor);
	
	return ActionResult<CTFBot>::Continue();
}
