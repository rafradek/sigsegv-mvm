/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/scenario/payload/tf_bot_payload_push.cpp
 * used in MvM: TODO
 */


ConVar tf_bot_cart_push_radius("tf_bot_cart_push_radius", "60", FCVAR_CHEAT);


CTFBotPayloadPush::CTFBotPayloadPush()
{
}

CTFBotPayloadPush::~CTFBotPayloadPush()
{
}


const char *CTFBotPayloadPush::GetName() const
{
	return "PayloadPush";
}


ActionResult<CTFBot> CTFBotPayloadPush::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
	this->m_PathFollower.Invalidate();
	
	// float @ 0x4814 = 180.0f
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotPayloadPush::Update(CTFBot *actor, float dt)
{
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
	if (threat != nullptr && threat->IsVisibleRecently()) {
		actor->EquipBestWeaponForThreat(threat);
	}
	
	if (TFGameRules()->InSetup()) {
		this->m_PathFollower.Invalidate();
		this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
		
		return ActionResult<CTFBot>::Continue();
	}
	
	CHandle<CTeamTrainWatcher> watcher = TFGameRules()->GetPayloadToPush(actor->GetTeamNumber());
	if (watcher == nullptr) {
		return ActionResult<CTFBot>::Continue();
	}
	
	CBaseEntity *train = watcher->GetTrainEntity();
	if (train == nullptr) {
		return ActionResult<CTFBot>::Continue();
	}
	
	if (this->m_ctRecomputePath.IsElapsed()) {
		VPROF_BUDGET("CTFBotPayloadPush::Update( repath )", "NextBot");
		
		Vector fwd;
		train->GetVectors(&fwd, nullptr, nullptr);
		
		Vector goal = train->WorldSpaceCenter() - (fwd * tf_bot_cart_push_radius.GetFloat());
		
		threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
		if (threat != nullptr) {
			Vector dir = train->WorldSpaceCenter() - threat->GetLastKnownPosition();
			VectorNormalize(dir);
			
			goal = train->WorldSpaceCenter() - (dir * tf_bot_cart_push_radius.GetFloat());
		}
		
		CTFBotPathCost cost_func(actor, DEFAULT_ROUTE);
		this->m_PathFollower.Compute(actor, goal, cost_func, 0.0f, true);
		
		this->m_ctRecomputePath.Start(RandomFloat(0.2f, 0.4f));
	}
	
	this->m_PathFollower.Update(actor);
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotPayloadPush::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	VPROF_BUDGET("CTFBotPayloadPush::OnResume", "NextBot");
	
	this->m_ctRecomputePath.Invalidate();
	
	return ActionResult<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotPayloadPush::OnMoveToSuccess(CTFBot *actor, const Path *path)
{
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotPayloadPush::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	VPROF_BUDGET("CTFBotPayloadPush::OnMoveToFailure", "NextBot");
	
	this->m_ctRecomputePath.Invalidate();
	
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotPayloadPush::OnStuck(CTFBot *actor)
{
	VPROF_BUDGET("CTFBotPayloadPush::OnStuck", "NextBot");
	
	this->m_ctRecomputePath.Invalidate();
	actor->GetLocomotionInterface()->ClearStuckStatus("");
	
	return EventDesiredResult<CTFBot>::Continue();
}


QueryResponse CTFBotPayloadPush::ShouldHurry(const INextBot *nextbot) const
{
	return QueryResponse::DONTCARE;
}

QueryResponse CTFBotPayloadPush::ShouldRetreat(const INextBot *nextbot) const
{
	return QueryResponse::DONTCARE;
}
