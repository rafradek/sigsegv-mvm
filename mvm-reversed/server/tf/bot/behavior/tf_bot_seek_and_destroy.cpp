/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_seek_and_destroy.cpp
 * used in MvM: TODO
 * 
 * Initial contained action of CTFBotScenarioMonitor (fallback for modes without specific actions)
 * SuspendFor from CTFBotTacticalMonitor::Update     (chase down losers after winning round)
 * SuspendFor from CTFBotCapturePoint::Update        (too early to capture)
 * SuspendFor from CTFBotCapturePoint::Update        (no points available yet)
 * SuspendFor from CTFBotDefendPoint::Update         (point is locked)
 * SuspendFor from CTFBotDefendPoint::Update         (no points available yet)
 * SuspendFor from CTFBotDefendPoint::Update         (attacking while ubered)
 * SuspendFor from CTFBotDefendPoint::Update         (lots of time on the clock)
 * SuspendFor from CTFBotDefendPoint::Update         (force pyro to chase enemies down)
 */


ConVar tf_bot_debug_seek_and_destroy("tf_bot_debug_seek_and_destroy", "0", FCVAR_CHEAT);


CTFBotSeekAndDestroy::CTFBotSeekAndDestroy(float duration)
{
	if (duration > 0.0f) {
		this->m_ctActionDuration.Start(duration);
	}
}

CTFBotSeekAndDestroy::~CTFBotSeekAndDestroy()
{
}


const char *CTFBotSeekAndDestroy::GetName() const
{
	return "SeekAndDestroy";
}


ActionResult<CTFBot> CTFBotSeekAndDestroy::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
	
	this->RecomputeSeekPath(actor);
	
	CTeamControlPoint *point = actor->GetMyControlPoint();
	if (point != nullptr) {
		this->m_bPointLocked = point->IsLocked();
	} else {
		this->m_bPointLocked = false;
	}
	
	/* start the countdown timer back to the beginning */
	if (this->m_ctActionDuration.HasStarted()) {
		this->m_ctActionDuration.Reset();
	}
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSeekAndDestroy::Update(CTFBot *actor, float dt)
{
	if (this->m_ctActionDuration.HasStarted() && this->m_ctActionDuration.IsElapsed()) {
		return ActionResult<CTFBot>::Done("Behavior duration elapsed");
	}
	
	if (TFGameRules()->IsInTraining() && actor->IsAnyPointBeingCaptured()) {
		return ActionResult<CTFBot>::Done("Assist trainee in capturing the point");
	}
	
	if (actor->IsCapturingPoint()) {
		return ActionResult<CTFBot>::Done("Keep capturing point I happened to stumble upon");
	}
	
	if (this->m_bPointLocked) {
		CTeamControlPoint *point = actor->GetMyControlPoint();
		if (point != nullptr && !point->IsLocked()) {
			return ActionResult<CTFBot>::Done("The point just unlocked");
		}
	}
	
	/* BUG: this condition always resolves as true if there's no round timer */
	if (TFGameRules()->State_Get() != GR_STATE_TEAM_WIN &&
		actor->GetTimeLeftToCapture() < tf_bot_offense_must_push_time.GetFloat()) {
		return ActionResult<CTFBot>::Done("Time to push for the objective");
	}
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
	if (threat != nullptr) {
		/* BUG: doesn't check if we're on the winning team; basically we just
		 * assume that we would only be in SeekAndDestroy in the first place if
		 * TacticalMonitor put us here */
		if (TFGameRules()->State_Get() == GR_STATE_TEAM_WIN) {
			return ActionResult<CTFBot>::SuspendFor(new CTFBotAttack(),
				"Chasing down the losers");
		}
		
		if (actor->IsRangeLessThan(threat->GetLastKnownPosition(), 1000.0f)) {
			return ActionResult<CTFBot>::SuspendFor(new CTFBotAttack(),
				"Going after an enemy");
		}
	}
	
	this->m_PathFollower.Update(actor);
	
	if (!this->m_PathFollower.IsValid() && this->m_ctRecomputePath.IsElapsed()) {
		this->m_ctRecomputePath.Start(1.0f);
		this->RecomputeSeekPath(actor);
	}
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSeekAndDestroy::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	this->RecomputeSeekPath(actor);
	
	return ActionResult<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotSeekAndDestroy::OnMoveToSuccess(CTFBot *actor, const Path *path)
{
	this->RecomputeSeekPath(actor);
	
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotSeekAndDestroy::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	this->RecomputeSeekPath(actor);
	
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotSeekAndDestroy::OnStuck(CTFBot *actor)
{
	this->RecomputeSeekPath(actor);
	
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotSeekAndDestroy::OnTerritoryContested(CTFBot *actor, int i1)
{
	return EventDesiredResult<CTFBot>::Done("Defending the point", ResultSeverity::MEDIUM);
}

EventDesiredResult<CTFBot> CTFBotSeekAndDestroy::OnTerritoryCaptured(CTFBot *actor, int i1)
{
	return EventDesiredResult<CTFBot>::Done("Giving up due to point capture", ResultSeverity::MEDIUM);
}

EventDesiredResult<CTFBot> CTFBotSeekAndDestroy::OnTerritoryLost(CTFBot *actor, int i1)
{
	return EventDesiredResult<CTFBot>::Done("Giving up due to point lost", ResultSeverity::MEDIUM);
}


QueryResponse CTFBotSeekAndDestroy::ShouldHurry(const INextBot *nextbot) const
{
	return QueryResponse::DONTCARE;
}

QueryResponse CTFBotSeekAndDestroy::ShouldRetreat(const INextBot *nextbot) const
{
	if (nextbot->GetEntity()->IsPlayerClass(TF_CLASS_PYRO)) {
		return QueryResponse::NO;
	} else {
		return QueryResponse::DONTCARE;
	}
}


CTFNavArea *CTFBotSeekAndDestroy::ChooseGoalArea(CTFBot *actor)
{
	CUtlVector<CTFNavArea *> areas;
	TheNavMesh->CollectSpawnRoomThresholdAreas(&areas, GetEnemyTeam(actor));
	
	CTeamControlPoint *point = actor->GetMyControlPoint();
	if (point != nullptr && !point->IsLocked()) {
		int index = point->GetPointIndex();
		if (index < 8) {
			// TODO: some CTFNavMesh stuff
			// (may add one more area to vector)
		}
	}
	
	if (tf_bot_debug_seek_and_destroy.GetBool()) {
		FOR_EACH_VEC(areas, i) {
			TheNavMesh->AddToSelectedSet(areas[i]);
		}
	}
	
	if (!areas.IsEmpty()) {
		return areas.Random();
	} else {
		return nullptr;
	}
}

void CTFBotSeekAndDestroy::RecomputeSeekPath(CTFBot *actor)
{
	if ((this->m_GoalArea = this->ChooseGoalArea(actor)) == nullptr) {
		this->m_PathFollower.Invalidate();
		return;
	}
	
	CTFBotPathCost path_cost(actor, SAFEST_ROUTE);
	this->m_PathFollower.Compute(actor, this->m_GoalArea->GetCenter(), path_cost, 0.0, true);
}


// TODO: need CTeamControlPoint *CTFBot::GetMyControlPoint
