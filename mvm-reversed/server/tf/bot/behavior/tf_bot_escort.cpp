/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_escort.cpp
 * used in MvM: TODO
 * 
 * completely unused
 */


ConVar tf_bot_escort_range("tf_bot_escort_range", "300", FCVAR_CHEAT);


CTFBotEscort::CTFBotEscort(CBaseEntity *who)
{
	this->m_hWho = who;
}

CTFBotEscort::~CTFBotEscort()
{
}


const char *CTFBotEscort::GetName() const
{
	return "Escort";
}


ActionResult<CTFBot> CTFBotEscort::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotEscort::Update(CTFBot *actor, float dt)
{
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
	if (threat != nullptr && threat->IsVisibleInFOVNow()) {
		return ActionResult<CTFBot>::SuspendFor(new CTFBotAttack(),
			"Attacking nearby threat");
	}
	
	if (this->m_hWho != nullptr) {
		if (actor->IsRangeGreaterThan(this->m_hWho, tf_bot_escort_range.GetFloat())) {
			if (this->m_ctRecomputePath.IsElapsed()) {
				CTFBotPathCost cost_func(actor, FASTEST_ROUTE);
				this->m_PathFollower.Compute(actor, this->m_hWho, cost_func, 0.0f, true);
				
				this->m_ctRecomputePath.Start(RandomFloat(2.0f, 3.0f));
			}
			
			this->m_PathFollower.Update(actor);
		} else if (CTFBotPrepareStickybombTrap::IsPossible(actor)) {
			return ActionResult<CTFBot>::SuspendFor(new CTFBotPrepareStickybombTrap(),
				"Laying sticky bombs!");
		}
	}
	
	if (actor->m_hTargetSentry != nullptr && CTFBotDestroyEnemySentry::IsPossible(actor)) {
		return ActionResult<CTFBot>::SuspendFor(new CTFBotDestroyEnemySentry(),
			"Going after an enemy sentry to destroy it");
	}
	
	return ActionResult<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotEscort::OnMoveToSuccess(CTFBot *actor, const Path *path)
{
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotEscort::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotEscort::OnStuck(CTFBot *actor)
{
	this->m_ctRecomputePath.Invalidate();
	
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotEscort::OnCommandApproach(CTFBot *actor, const Vector& v1, float f1)
{
	return EventDesiredResult<CTFBot>::Continue();
}


QueryResponse CTFBotEscort::ShouldRetreat(const INextBot *nextbot) const
{
	return QueryResponse::NO;
}


CBaseEntity *CTFBotEscort::GetWho() const
{
	return this->m_hWho;
}

void CTFBotEscort::SetWho(CBaseEntity *who)
{
	this->m_hWho = who;
}
