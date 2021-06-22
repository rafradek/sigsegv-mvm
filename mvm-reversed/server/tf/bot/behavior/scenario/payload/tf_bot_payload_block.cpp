/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/scenario/payload/tf_bot_payload_block.cpp
 * used in MvM: TODO
 */


CTFBotPayloadBlock::CTFBotPayloadBlock()
{
}

CTFBotPayloadBlock::~CTFBotPayloadBlock()
{
}

	
const char *CTFBotPayloadBlock::GetName() const
{
	return "PayloadBlock";
}


ActionResult<CTFBot> CTFBotPayloadBlock::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
	this->m_PathFollower.Invalidate();
	
	this->m_ctBlockDuration.Start(RandomFloat(3.0f, 5.0f));
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotPayloadBlock::Update(CTFBot *actor, float dt)
{
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
	if (threat != nullptr && threat->IsVisibleRecently()) {
		actor->EquipBestWeaponForThreat(threat);
	}
	
	if (this->m_ctBlockDuration.IsElapsed()) {
		return ActionResult<CTFBot>::Done("Been blocking long enough");
	}
	
	if (this->m_ctRecomputePath.IsElapsed()) {
		VPROF_BUDGET("CTFBotPayloadBlock::Update( repath )", "NextBot");
		
		CHandle<CTeamTrainWatcher> watcher = TFGameRules()->GetPayloadToBlock(actor->GetTeamNumber());
		if (watcher == nullptr) {
			return ActionResult<CTFBot>::Done("Train Watcher is missing");
		}
		
		CBaseEntity *train = watcher->GetTrainEntity();
		if (train == nullptr) {
			return ActionResult<CTFBot>::Done("Cart is missing");
		}
		
		CTFBotPathCost cost_func(actor, DEFAULT_ROUTE);
		this->m_PathFollower.Compute(actor, train->WorldSpaceCenter(), cost_func, 0.0f, true);
		
		this->m_ctRecomputePath.Start(RandomFloat(0.2f, 0.4f));
	}
	
	this->m_PathFollower.Update(actor);
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotPayloadBlock::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	VPROF_BUDGET("CTFBotPayloadBlock::OnResume", "NextBot");
	
	this->m_ctRecomputePath.Invalidate();
	
	return ActionResult<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotPayloadBlock::OnMoveToSuccess(CTFBot *actor, const Path *path)
{
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotPayloadBlock::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	VPROF_BUDGET("CTFBotPayloadBlock::OnMoveToFailure", "NextBot");
	
	this->m_ctRecomputePath.Invalidate();
	
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotPayloadBlock::OnStuck(CTFBot *actor)
{
	VPROF_BUDGET("CTFBotPayloadBlock::OnStuck", "NextBot");
	
	this->m_ctRecomputePath.Invalidate();
	actor->GetLocomotionInterface()->ClearStuckStatus("");
	
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotPayloadBlock::OnTerritoryContested(CTFBot *actor, int i1)
{
	return EventDesiredResult<CTFBot>::Sustain(nullptr, ResultSeverity::MEDIUM);
}

EventDesiredResult<CTFBot> CTFBotPayloadBlock::OnTerritoryCaptured(CTFBot *actor, int i1)
{
	return EventDesiredResult<CTFBot>::Sustain(nullptr, ResultSeverity::MEDIUM);
}

EventDesiredResult<CTFBot> CTFBotPayloadBlock::OnTerritoryLost(CTFBot *actor, int i1)
{
	return EventDesiredResult<CTFBot>::Sustain(nullptr, ResultSeverity::MEDIUM);
}


QueryResponse CTFBotPayloadBlock::ShouldHurry(const INextBot *nextbot) const
{
	return QueryResponse::YES;
}

QueryResponse CTFBotPayloadBlock::ShouldRetreat(const INextBot *nextbot) const
{
	return QueryResponse::DONTCARE;
}
