/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_retreat_to_cover.cpp
 * used in MvM: TODO
 * 
 * SuspendFor from CTFBotTacticalMonitor::Update
 * SuspendFor from CTFBotTacticalMonitor::Update
 * SuspendFor from CTFBotTacticalMonitor::Update
 * SuspendFor from CTFBotDestroyEnemySentry::Update
 * SuspendFor from CTFBotDestroyEnemySentry::Update
 * SuspendFor from CTFBotUberAttackEnemySentry::Update
 * SuspendFor from CTFBotUberAttackEnemySentry::Update
 * SuspendFor from CTFBotMvMEngineerIdle::Update
 * SuspendFor from CTFBotSniperAttack::Update
 * SuspendFor from CTFBotSniperAttack::Update
 * ChangeTo from CTFBotSpyAttack::Update
 * ChangeTo from CTFBotSpyAttack::OnInjured
 * SuspendFor from CTFBotSpyInfiltrate::Update       (afterward: CTFBotSpyAttack)
 * SuspendFor from CTFBotEngineerMoveToBuild::Update (afterward: CTFBotEngineerBuildTeleportExit)
 */


ConVar tf_bot_retreat_to_cover_range("tf_bot_retreat_to_cover_range", "1000", FCVAR_CHEAT);
ConVar tf_bot_debug_retreat_to_cover("tf_bot_debug_retreat_to_cover", "0", FCVAR_CHEAT);
ConVar tf_bot_wait_in_cover_min_time("tf_bot_wait_in_cover_min_time", "1", FCVAR_CHEAT);
ConVar tf_bot_wait_in_cover_max_time("tf_bot_wait_in_cover_max_time", "2", FCVAR_CHEAT);


CTFBotRetreatToCover::CTFBotRetreatToCover(Action<CTFBot *> *done_action)
{
	this->m_flDuration = -1.0f;
	this->m_DoneAction = done_action;
}

CTFBotRetreatToCover::CTFBotRetreatToCover(float duration)
{
	this->m_flDuration = duration;
	this->m_DoneAction = nullptr;
}

CTFBotRetreatToCover::~CTFBotRetreatToCover()
{
}


const char *CTFBotRetreatToCover::GetName() const
{
	return "RetreatToCover";
}


ActionResult<CTFBot> CTFBotRetreatToCover::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
	
	this->m_CoverArea = this->FindCoverArea(actor);
	if (this->m_CoverArea == nullptr) {
		return ActionResult<CTFBot>::Done("No cover available!");
	}
	
	if (this->m_flDuration < 0.0f) {
		this->m_flDuration = RandomFloat(tf_bot_wait_in_cover_min_time.GetFloat(),
			tf_bot_wait_in_cover_max_time.GetFloat());
	}
	
	this->m_ctActionDuration.Start(this->m_flDuration);
	
	if (actor->IsPlayerClass(TF_CLASS_SPY) && !actor->m_Shared.IsStealthed()) {
		actor->PressAltFireButton();
	}
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotRetreatToCover::Update(CTFBot *actor, float dt)
{
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(true);
	
	if (actor->m_Shared.InCond(TF_COND_INVULNERABLE)) {
		return ActionResult<CTFBot>::Done("I'm invulnerable - no need to reatreat!");
	}
	
	/* does this even query the right thing? */
	if (!this->ShouldRetreat(actor)) {
		return ActionResult<CTFBot>::Done("No longer need to retreat");
	}
	
	actor->EquipBestWeaponForThreat(threat);
	
	auto primary = static_cast<CTFWeaponBase *>(actor->Weapon_GetSlot(0));
	
	bool reloading = false;
	if (primary != nullptr && actor->GetAmmoCount(TF_AMMO_PRIMARY) > 0 && actor->IsBarrageAndReloadWeapon(primary) &&
		primary->Clip1() < primary->GetMaxClip1()) {
		actor->PressReloadButton();
		reloading = true;
	}
	
	if (actor->GetLastKnownArea() == this->m_CoverArea && threat != nullptr) {
		this->m_CoverArea = this->FindCoverArea(actor);
		if (this->m_CoverArea == nullptr) {
			return ActionResult<CTFBot>::Done("My cover is exposed, and there is no other cover available!");
		}
	}
	
	if (threat != nullptr) {
		this->m_ctActionDuration.Reset();
		
		if (this->m_ctRecomputePath.IsElapsed()) {
			this->m_ctRecomputePath.Start(RandomFloat(0.3f, 0.5f));
			
			CTFBotPathCost cost_func(actor, RETREAT_ROUTE);
			this->m_PathFollower.Compute(actor, this->m_CoverArea->GetCenter(), cost_func, 0.0f, true);
		}
		
		this->m_PathFollower.Update(actor);
		
		return ActionResult<CTFBot>::Continue();
	}
	
	if (actor->IsPlayerClass(TF_CLASS_SPY) && actor->m_Shared.InCond(TF_COND_DISGUISED)) {
		return ActionResult<CTFBot>::Continue();
	}
	
	if (actor->m_Shared.IsStealthed()) {
		actor->PressAltFireButton();
	}
	
	if (this->m_DoneAction != nullptr) {
		return ActionResult<CTFBot>::ChangeTo(this->m_DoneAction,
			"Doing given action now that I'm in cover");
	}
	
	// 0x2190: actor->m_Shared.m_nNumHealers
	
	// TODO
}


EventDesiredResult<CTFBot> CTFBotRetreatToCover::OnMoveToSuccess(CTFBot *actor, const Path *path)
{
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotRetreatToCover::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotRetreatToCover::OnStuck(CTFBot *actor)
{
	return EventDesiredResult<CTFBot>::Continue();
}


QueryResponse CTFBotRetreatToCover::ShouldHurry(const INextBot *nextbot) const
{
	return QueryResponse::YES;
}


CTFNavArea *CTFBotRetreatToCover::FindCoverArea(CTFBot *actor)
{
	VPROF_BUDGET("CTFBotRetreatToCover::FindCoverArea", "NextBot");
	
	// TODO
}


CSearchForCover::CSearchForCover(/* ??? */)
{
	// TODO
}

CSearchForCover::~CSearchForCover()
{
}


bool CSearchForCover::operator()(CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar)
{
	VPROF_BUDGET("CSearchForCover::operator()", "NextBot");
	
	CTestAreaAgainstThreats functor(/* TODO */);
	this->m_Actor->GetVisionInterface()->ForEachKnownEntity(functor);
	
	// TODO
}

bool CSearchForCover::ShouldSearch(CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar)
{
	if (travelDistanceSoFar > tf_bot_retreat_to_cover_range.GetFloat()) {
		return false;
	}
	
	return (this->m_Actor->GetLocomotionInterface()->GetStepHeight() >
		currentArea->ComputeAdjacentConnectionHeightChange(adjArea))
}

void CSearchForCover::PostSearch()
{
	if (tf_bot_debug_retreat_to_cover.GetBool()) {
		FOR_EACH_VEC(this->m_Areas, i) {
			TheNavMesh->AddToSelectedSet(this->m_Areas[i]);
		}
	}
}


CTestAreaAgainstThreats::CTestAreaAgainstThreats(/* ??? */)
{
	// TODO
}

CTestAreaAgainstThreats::~CTestAreaAgainstThreats()
{
}


bool CTestAreaAgainstThreats::Inspect(const CKnownEntity& known)
{
	VPROF_BUDGET("CTestAreaAgainstThreats::Inspect", "NextBot");
	
	if (this->m_Actor->IsEnemy(known.GetEntity())) {
		CNavArea *lastknown = this->m_Actor->GetLastKnownArea();
		if (lastknown != nullptr && this->m_Area->IsPotentiallyVisible(lastknown)) {
			++this->m_nVisible;
		}
	}
	
	return true;
}
