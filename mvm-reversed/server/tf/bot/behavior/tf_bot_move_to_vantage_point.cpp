/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_move_to_vantage_point.cpp
 * used in MvM: TODO
 * 
 * completely unused
 */


CTFBotMoveToVangatePoint::CTFBotMoveToVantagePoint(float max_cost)
{
	this->m_flMaxCost = max_cost;
}

CTFBotMoveToVangatePoint::~CTFBotMoveToVantagePoint()
{
}


const char *CTFBotMoveToVangatePoint::GetName() const
{
	return "MoveToVantagePoint";
}


ActionResult<CTFBot> CTFBotMoveToVangatePoint::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
	
	this->m_VantagePoint = actor->FindVantagePoint(this->m_flMaxCost);
	if (this->m_VantagePoint == nullptr) {
		return ActionResult<CTFBot>::Done("No vantage point found");
	}
	
	this->m_PathFollower.Invalidate();
	this->m_ctRecomputePath.Invalidate();
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotMoveToVangatePoint::Update(CTFBot *actor, float dt)
{
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
	if (threat != nullptr && threat->IsVisibleInFOVNow()) {
		return ActionResult<CTFBot>::Done("Enemy is visible");
	}
	
	if (!this->m_PathFollower.IsValid() || this->m_ctRecomputePath.IsElapsed()) {
		this->m_ctRecomputePath.Start(1.0f);
		
		CTFBotPathCost cost_func(actor, FASTEST_ROUTE);
		if (!this->m_PathFollower.Compute(actor, this->m_VantagePoint->GetCenter(), cost_func, 0.0f, true)) {
			return ActionResult<CTFBot>::Done("No path to vantage point exists");
		}
	}
	
	this->m_PathFollower.Update(actor);
	
	return ActionResult<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotMoveToVangatePoint::OnMoveToSuccess(CTFBot *actor, const Path *path)
{
	return EventDesiredResult<CTFBot>::Done("Vantage point reached");
}

EventDesiredResult<CTFBot> CTFBotMoveToVangatePoint::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	this->m_PathFollower.Invalidate();
	
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotMoveToVangatePoint::OnStuck(CTFBot *actor)
{
	this->m_PathFollower.Invalidate();
	
	return EventDesiredResult<CTFBot>::Continue();
}
