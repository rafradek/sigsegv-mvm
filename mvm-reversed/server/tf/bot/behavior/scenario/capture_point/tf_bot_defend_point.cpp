/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/scenario/capture_point/tf_bot_defend_point.cpp
 * used in MvM: TODO
 */


ConVar tf_bot_defense_must_defend_time("tf_bot_defense_must_defend_time", "300", FCVAR_CHEAT,
	"If timer is less than this, bots will stay near point and guard");
ConVar tf_bot_max_point_defend_range("tf_bot_max_point_defend_range", "1250", FCVAR_CHEAT,
	"How far (in travel distance) from the point defending bots will take up positions");
ConVar tf_bot_defense_debug("tf_bot_defense_debug", "0", FCVAR_CHEAT);


CTFBotDefendPoint::CTFBotDefendPoint()
{
}

CTFBotDefendPoint::~CTFBotDefendPoint()
{
}


const char *CTFBotDefendPoint::GetName() const
{
	return "DefendPoint";
}


ActionResult<CTFBot> CTFBotDefendPoint::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	static const float roamChance[] = {
		10.0f, // easy
		50.0f, // normal
		75.0f, // hard
		90.0f, // expert
	};
	
	this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
	
	this->m_bShouldRoam = (RandomFloat(0.0f, 100.0f) < roamChance[Clamp(actor->m_iSkill, 0, 3)]);
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotDefendPoint::Update(CTFBot *actor, float dt)
{
	if (!g_hControlPointMasters.IsEmpty()) {
		CTeamControlPointMaster *master = g_hControlPointMasters[0];
		if (master != nullptr && master->GetNumPoints() == 1) {
			CTeamControlPoint *point = master->GetControlPoint(0);
			if (point != nullptr && point->GetPointIndex() == 0 &&
				point->GetOwner() != actor->GetTeamNumber()) {
				return ActionResult<CTFBot>::ChangeTo(new CTFBotCapturePoint(),
					"We need to capture the point!");
			}
		}
	}
	
	CTeamControlPoint *point = actor->GetMyControlPoint();
	if (point == nullptr) {
		return ActionResult<CTFBot>::SuspendFor(new CTFBotSeekAndDestroy(10.0f),
			"Seek and destroy until a point becomes available");
	}
	
	if (point->GetTeamNumber() != actor->GetTeamNumber()) {
		return ActionResult<CTFBot>::ChangeTo(new CTFBotCapturePoint(),
			"We need to capture our point(s)");
	}
	
	if (this->IsPointThreatened(actor) && this->WillBlockCapture(actor)) {
		return ActionResult<CTFBot>::SuspendFor(new CTFBotDefendPointBlockCapture(),
			"Moving to block point capture!");
	}
	
	/* BUG: should be IsInvulnerable, to cover other uber conds */
	if (actor->m_Shared.InCond(TF_COND_INVULNERABLE)) {
		return ActionResult<CTFBot>::SuspendFor(new CTFBotSeekAndDestroy(6.0f),
			"Attacking because I'm uber'd!");
	}
	
	if (point->IsLocked()) {
		return ActionResult<CTFBot>::SuspendFor(new CTFBotSeekAndDestroy(),
			"Seek and destroy until the point unlocks");
	}
	
	if (this->m_bShouldRoam && actor->GetTimeLeftToCapture() > tf_bot_defense_must_defend_time.GetFloat()) {
		return ActionResult<CTFBot>::SuspendFor(new CTFBotSeekAndDestroy(15.0f),
			"Seek and destroy - we have lots of time");
	}
	
	if (TFGameRules()->InSetup()) {
		// CT @ 0x9020 .Reset()
	}
	
	
	
	// TODO
}

ActionResult<CTFBot> CTFBotDefendPoint::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	actor->ClearMyControlPoint();
	
	// TODO: invalidate CT @ 0x9008
	this->m_PathFollower.Invalidate();
	
	return ActionResult<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotDefendPoint::OnContact(CTFBot *actor, CBaseEntity *ent, CGameTrace *trace)
{
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotDefendPoint::OnMoveToSuccess(CTFBot *actor, const Path *path)
{
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotDefendPoint::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	this->m_PathFollower.Invalidate();
	this->m_DefenseArea = this->SelectAreaToDefendFrom(actor);
	
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotDefendPoint::OnStuck(CTFBot *actor)
{
	this->m_PathFollower.Invalidate();
	this->m_DefenseArea = this->SelectAreaToDefendFrom(actor);
	actor->GetLocomotionInterface()->ClearStuckStatus("");
	
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotDefendPoint::OnTerritoryContested(CTFBot *actor, int i1)
{
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotDefendPoint::OnTerritoryCaptured(CTFBot *actor, int i1)
{
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotDefendPoint::OnTerritoryLost(CTFBot *actor, int i1)
{
	actor->ClearMyControlPoint();
	
	this->m_DefenseArea = this->SelectAreaToDefendFrom(actor);
	// TODO: invalidate CT @ 0x9008
	
	this->m_PathFollower.Invalidate();
	
	return EventDesiredResult<CTFBot>::Continue();
}


bool CTFBotDefendPoint::IsPointThreatened(CTFBot *actor)
{
	CTeamControlPoint *point = actor->GetMyControlPoint();
	if (point == nullptr) {
		return false;
	}
	
	if (!point->HasBeenContested() || gpGlobals->curtime - point->LastContestedAt() >= 5.0f) {
		if (/* CT @ CTFBot+0x2b04 !HasStarted || IsElapsed */) {
			return false;
		}
	}
	
	return true;
}

CTFNavArea *CTFBotDefendPoint::SelectAreaToDefendFrom(CTFBot *actor)
{
	VPROF_BUDGET("CTFBotDefendPoint::SelectAreaToDefendFrom", "NextBot");
	
	CTeamControlPoint *point = actor->GetMyControlPoint();
	if (point == nullptr) {
		return nullptr;
	}
	
	if (point->GetPointIndex() > 7) {
		return nullptr;
	}
	
	
	
	// TODO
}

bool CTFBotDefendPoint::WillBlockCapture(CTFBot *actor) const
{
	if (TFGameRules()->IsInTraining()) {
		return false;
	}
	
	switch (actor->m_iSkill) {
	case CTFBot::DifficultyType::EASY:
		return false;
	case CTFBot::DifficultyType::NORMAL:
		return (actor->TransientlyConsistentRandomValue(10.0f, 0) > 0.5f);
	default:
		return true;
	}
}


bool CSelectDefenseAreaForPoint::operator()(CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar)
{
	
	
	// TODO
}

bool CSelectDefenseAreaForPoint::ShouldSearch(CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar)
{
	int team = (TFGameRules()->IsInKothMode() ? TEAM_ANY : this->m_iTeamNum);
	
	if (adjArea->IsBlocked(team) || travelDistanceSoFar > tf_bot_max_point_defend_range.GetFloat()) {
		return false;
	}
	
	if (abs(currentArea->ComputeAdjacentConnectionHeightChange(adjArea)) < 65.0f) {
		return true;
	} else {
		return false;
	}
}
