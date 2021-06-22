/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/scenario/capture_point/tf_bot_capture_point.cpp
 * used in MvM: TODO
 */


ConVar tf_bot_offense_must_push_time("tf_bot_offense_must_push_time", "120", FCVAR_CHEAT,
	"If timer is less than this, bots will push hard to cap");
ConVar tf_bot_capture_seek_and_destroy_min_duration("tf_bot_capture_seek_and_destroy_min_duration", "15", FCVAR_CHEAT,
	"If a capturing bot decides to go hunting, this is the min duration he will hunt for before reconsidering");
ConVar tf_bot_capture_seek_and_destroy_max_duration("tf_bot_capture_seek_and_destroy_max_duration", "30", FCVAR_CHEAT,
	"If a capturing bot decides to go hunting, this is the max duration he will hunt for before reconsidering");


CTFBotCapturePoint::CTFBotCapturePoint()
{
}

CTFBotCapturePoint::~CTFBotCapturePoint()
{
}


const char *CTFBotCapturePoint::GetName() const
{
	return "CapturePoint";
}


ActionResult<CTFBot> CTFBotCapturePoint::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	VPROF_BUDGET("CTFBotCapturePoint::OnStart", "NextBot");
	
	this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
	this->m_PathFollower.Invalidate();
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotCapturePoint::Update(CTFBot *actor, float dt)
{
	if (TFGameRules()->InSetup()) {
		this->m_PathFollower.Invalidate();
		this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
		
		return ActionResult<CTFBot>::Continue();
	}
	
	CTeamControlPoint *point = actor->GetMyControlPoint();
	if (point == nullptr) {
		return ActionResult<CTFBot>::SuspendFor(new CTFBotSeekAndDestroy(10.0f),
			"Seek and destroy until a point becomes available");
	}
	
	if (point->GetTeamNumber() == actor->GetTeamNumber()) {
		return ActionResult<CTFBot>::ChangeTo(new CTFBotDefendPoint(),
			"We need to defend our point(s)");
	}
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
	if (threat != nullptr && threat->IsVisibleRecently()) {
		actor->EquipBestWeaponForThreat(threat);
	}
	
	if ((!actor->IsPointBeingCaptured(point) || actor->GetTimeSinceWeaponFired() < 2.0f) &&
		!actor->IsCapturingPoint() && !TFGameRules()->InOvertime() &&
		actor->GetTimeLeftToCapture() >= tf_bot_offense_must_push_time.GetFloat() &&
		!TFGameRules()->IsInTraining() && !actor->IsNearPoint(point) &&
		threat != nullptr && threat->IsVisibleRecently()) {
		float duration = RandomFloat(tf_bot_capture_seek_and_destroy_min_duration.GetFloat(),
			tf_bot_capture_seek_and_destroy_max_duration.GetFloat());
		
		return ActionResult<CTFBot>::SuspendFor(new CTFBotSeekAndDestroy(duration),
			"Too early to capture - hunting");
	}
	
	if (actor->IsCapturingPoint()) {
		if (point->GetPointIndex() > 7) {
			return ActionResult<CTFBot>::Continue();
		}
		
		// TODO
		
		// ...
		
		// Path::Compute
	}
	
	if (this->m_ctRecomputePath.IsElapsed()) {
		VPROF_BUDGET("CTFBotCapturePoint::Update( repath )", "NextBot");
		
		CTFBotPathCost cost_func(actor, SAFEST_ROUTE);
		this->m_PathFollower.Compute(actor, point->GetAbsOrigin(), cost_func, 0.0f, true);
		
		this->m_ctRecomputePath.Start(RandomFloat(2.0f, 3.0f));
	}
	
	if (TFGameRules()->IsInTraining() && !actor->IsAnyPointBeingCaptured() &&
		this->m_PathFollower.GetLength() < 1000.0f) {
		actor->SpeakConceptIfAllowed(MP_CONCEPT_PLAYER_GO);
	} else {
		this->m_PathFollower.Update(actor);
	}
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotCapturePoint::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_ctRecomputePath.Invalidate();
	
	return ActionResult<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotCapturePoint::OnMoveToSuccess(CTFBot *actor, const Path *path)
{
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotCapturePoint::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	this->m_ctRecomputePath.Invalidate();
	
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotCapturePoint::OnStuck(CTFBot *actor)
{
	this->m_ctRecomputePath.Invalidate();
	actor->GetLocomotionInterface()->ClearStuckStatus("");
	
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotCapturePoint::OnTerritoryContested(CTFBot *actor, int i1)
{
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotCapturePoint::OnTerritoryCaptured(CTFBot *actor, int i1)
{
	this->m_ctRecomputePath.Invalidate();
	
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotCapturePoint::OnTerritoryLost(CTFBot *actor, int i1)
{
	return EventDesiredResult<CTFBot>::Continue();
}


QueryResponse CTFBotCapturePoint::ShouldHurry(const INextBot *nextbot) const
{
	auto actor = static_cast<CTFBot *>(nextbot->GetEntity());
	
	if (actor->GetTimeLeftToCapture() < tf_bot_offense_must_push_time.GetFloat()) {
		return QueryResponse::YES;
	}
	
	return QueryResponse::DONTCARE;
}

QueryResponse CTFBotCapturePoint::ShouldRetreat(const INextBot *nextbot) const
{
	auto actor = static_cast<CTFBot *>(nextbot->GetEntity());
	
	if (actor->GetTimeLeftToCapture() < tf_bot_offense_must_push_time.GetFloat()) {
		return QueryResponse::NO;
	}
	
	return QueryResponse::DONTCARE;
}
