/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/scenario/payload/tf_bot_payload_guard.cpp
 * used in MvM: TODO
 */


ConVar tf_bot_payload_guard_range("tf_bot_payload_guard_range", "1000", FCVAR_CHEAT);
ConVar tf_bot_debug_payload_guard_vantage_points("tf_bot_debug_payload_guard_vantage_points", nullptr, FCVAR_CHEAT);


CTFBotPayloadGuard::CTFBotPayloadGuard()
{
}

CTFBotPayloadGuard::~CTFBotPayloadGuard()
{
}


const char *CTFBotPayloadGuard::GetName() const
{
	return "PayloadGuard";
}


ActionResult<CTFBot> CTFBotPayloadGuard::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
	this->m_PathFollower.Invalidate();
	
	// Vector @ 0x4814 = actor->GetAbsOrigin()
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotPayloadGuard::Update(CTFBot *actor, float dt)
{
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
	if (threat != nullptr && threat->IsVisibleRecently()) {
		actor->EquipBestWeaponForThreat(threat);
	}
	
	CHandle<CTeamTrainWatcher> watcher = TFGameRules()->GetPayloadToBlock(actor->GetTeamNumber());
	if (watcher == nullptr) {
		return ActionResult<CTFBot>::Continue();
	}
	
	CBaseEntity *train = watcher->GetTrainEntity();
	if (train == nullptr) {
		return ActionResult<CTFBot>::Continue();
	}
	
	if (!watcher->IsDisabled() && watcher->GetCapturerCount() > 0) {
		// TODO
		// (there's some branchy stuff going on here)
	}
	
	// TODO
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotPayloadGuard::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	VPROF_BUDGET("CTFBotPayloadGuard::OnResume", "NextBot");
	
	// Invalidate CT @ 0x4828
	// Invalidate CT @ 0x4810
	
	return ActionResult<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotPayloadGuard::OnMoveToSuccess(CTFBot *actor, const Path *path)
{
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotPayloadGuard::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	VPROF_BUDGET("CTFBotPayloadGuard::OnMoveToFailure", "NextBot");
	
	// Invalidate CT @ 0x4828
	// Invalidate CT @ 0x4810
	
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotPayloadGuard::OnStuck(CTFBot *actor)
{
	VPROF_BUDGET("CTFBotPayloadGuard::OnStuck", "NextBot");
	
	// Invalidate CT @ 0x4810
	actor->GetLocomotionInterface()->ClearStuckStatus("");
	
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotPayloadGuard::OnTerritoryContested(CTFBot *actor, int i1)
{
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotPayloadGuard::OnTerritoryCaptured(CTFBot *actor, int i1)
{
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotPayloadGuard::OnTerritoryLost(CTFBot *actor, int i1)
{
	return EventDesiredResult<CTFBot>::Continue();
}


QueryResponse CTFBotPayloadGuard::ShouldHurry(const INextBot *nextbot) const
{
	return QueryResponse::DONTCARE;
}

QueryResponse CTFBotPayloadGuard::ShouldRetreat(const INextBot *nextbot) const
{
	CTFBot *actor = ToTFBot(nextbot->GetEntity());
	
	CHandle<CTeamTrainWatcher> watcher = TFGameRules()->GetPayloadToBlock(actor->GetTeamNumber());
	if (watcher != nullptr && watcher->IsTrainNearCheckpoint()) {
		return QueryResponse::NO;
	}
	
	return QueryResponse::DONTCARE;
}


Vector CTFBotPayloadGuard::FindVantagePoint(CTFBot *actor, CBaseEntity *target)
{
	CCollectPayloadGuardVantagePoints functor;
	SearchSurroundingAreas(TheNavMesh->GetNearestNavArea(target), functor, tf_bot_payload_guard_range.GetFloat());
	
	return functor.GetResult();
}


bool CCollectPayloadGuardVantagePoints::operator()(CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar)
{
	NextBotTraceFilterIgnoreActors filter(nullptr);
	
	for (int i = 3; i > 0; --i) {
		// HumanEyeHeight = 62.0f, from nav.h; value is from CS:S and is actually a bit wrong for TF2
		Vector point = area->GetRandomPoint() + Vector(0.0f, 0.0f, HumanEyeHeight);
		
		trace_t tr;
		UTIL_TraceLine(point, this->m_Target->WorldSpaceCenter(), MASK_SOLID_BRUSHONLY, &filter, &tr);
		
		if ((tr.fraction >= 1.0f && !tr.allsolid && !tr.startsolid) || tr.m_pEnt == this->m_Target) {
			this->m_VantagePoints.AddToTail(point);
			
			if (tf_bot_debug_payload_guard_vantage_points.GetBool()) {
				NDebugOverlay::Cross3D(point, 5.0f, 0xff, 0x00, 0xff, true, 120.0f);
			}
		}
	}
	
	return true;
}


Vector CCollectPayloadGuardVantagePoints::GetResult() const
{
	if (this->m_VantagePoints.IsEmpty()) {
		return this->m_Target->WorldSpaceCenter();
	}
	
	return this->m_VantagePoints.Random();
}
