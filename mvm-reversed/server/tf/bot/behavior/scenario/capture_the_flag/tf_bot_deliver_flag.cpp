/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/scenario/capture_the_flag/tf_bot_deliver_flag.cpp
 * used in MvM: TODO
 */


ConVar tf_mvm_bot_allow_flag_carrier_to_fight("tf_mvm_bot_allow_flag_carrier_to_fight", "1", FCVAR_CHEAT);
ConVar tf_mvm_bot_flag_carrier_interval_to_1st_upgrade("tf_mvm_bot_flag_carrier_interval_to_1st_upgrade", "5", FCVAR_CHEAT);
ConVar tf_mvm_bot_flag_carrier_interval_to_2nd_upgrade("tf_mvm_bot_flag_carrier_interval_to_2nd_upgrade", "15", FCVAR_CHEAT);
ConVar tf_mvm_bot_flag_carrier_interval_to_3rd_upgrade("tf_mvm_bot_flag_carrier_interval_to_3rd_upgrade", "15", FCVAR_CHEAT);
ConVar tf_mvm_bot_flag_carrier_health_regen("tf_mvm_bot_flag_carrier_health_regen", "45.0f", FCVAR_CHEAT);


CTFBotDeliverFlag::CTFBotDeliverFlag()
{
}

CTFBotDeliverFlag::~CTFBotDeliverFlag()
{
}


const char *CTFBotDeliverFlag::GetName() const
{
	return "DeliverFlag";
}


ActionResult<CTFBot> CTFBotDeliverFlag::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_ctRecomputePath.Invalidate();
	this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
	
	if (!tf_mvm_bot_allow_flag_carrier_to_fight.GetBool()) {
		actor->m_nBotAttrs |= CTFBot::AttributeType::SUPPRESSFIRE;
	}
	
	if (actor->IsMiniBoss()) {
		// 4824 = -1
		
		
	} else {
		
	}
	
	// TODO
}

ActionResult<CTFBot> CTFBotDeliverFlag::Update(CTFBot *actor, float dt)
{
	// TODO
	
	// NOTE: CTFBotDeliverFlag::Update
	// calls CTFBotPathCost::CTFBotPathCost
	// which is only in certain versions (e.g. ServerLinux 20140619a)
}

void CTFBotDeliverFlag::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
	/* BUG: will make bots that are intended to have SuppressFire lose it */
	actor->m_nBotAttrs &= ~CTFBot::AttributeType::SUPPRESSFIRE;
	
	if (TFGameRules() && TFGameRules()->IsMannVsMachineMode()) {
		// BUG: doesn't revert TF_COND_CRITBOOSTED_USER_BUFF if gained via flagcarrier upgrade
		actor->m_Shared.ResetSoldierBuffs();
	}
}


EventDesiredResult<CTFBot> CTFBotDeliverFlag::OnContact(CTFBot *actor, CBaseEntity *ent, CGameTrace *trace)
{
	if (TFGameRules()->IsMannVsMachineMode() && ent != nullptr && ent->ClassMatches("func_capturezone")) {
		return EventDesiredResult<CTFBot>::SuspendFor(new CTFBotMvMDeployBomb(), "Delivering the bomb!", ResultSeverity::CRITICAL);
	}
	
	return EventDesiredResult<CTFBot>::Continue();
}


QueryResponse CTFBotDeliverFlag::ShouldHurry(const INextBot *nextbot) const
{
	return QueryResponse::YES;
}

QueryResponse CTFBotDeliverFlag::ShouldRetreat(const INextBot *nextbot) const
{
	return QueryResponse::NO;
}

QueryResponse CTFBotDeliverFlag::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	// BUG: applies to non-MvM as well!
	if (tf_mvm_bot_allow_flag_carrier_to_fight.GetBool()) {
		return QueryResponse::YES;
	} else {
		return QueryResponse::NO;
	}
}


bool CTFBotDeliverFlag::UpgradeOverTime(CTFBot *actor)
{
	if (!TFGameRules()->IsMannVsMachineMode()) {
		return false;
	}
	
	if (this->int_0x4824 == -1) {
		return false;
	}
	
	CTFNavArea *area = actor->GetLastKnownArea();
	if (area != nullptr && (area->m_nAttributes & ((actor->GetTeamNumber() == TF_TEAM_RED) ? RED_SPAWN_ROOM : BLUE_SPAWN_ROOM)) != 0) {
		this->m_ctUnknown2.Start(tf_mvm_bot_flag_carrier_interval_to_1st_upgrade.GetFloat());
		TFObjectiveResource()->m_flMvMNextBombUpgradeTime = // TODO
		
		
	}
	
	// TODO
}


CTFBotPushToCapturePoint::CTFBotPushToCapturePoint(Action<CTFBot> *done_action)
{
	this->m_DoneAction = done_action;
}

CTFBotPushToCapturePoint::~CTFBotPushToCapturePoint()
{
}


const char *CTFBotPushToCapturePoint::GetName() const
{
	return "PushToCapturePoint";
}


ActionResult<CTFBot> CTFBotPushToCapturePoint::Update(CTFBot *actor, float dt)
{
	// TODO
}


EventDesiredResult<CTFBot> CTFBotPushToCapturePoint::OnNavAreaChanged(CTFBot *actor, CNavArea *area1, CNavArea *area2)
{
	if (area1 == nullptr || !area1->HasPrerequisite) {
		return EventDesiredResult<CTFBot>::Continue();
	}
	
	FOR_EACH_VEC(area1->GetPrerequisiteVector(), i) {
		CFuncNavPrerequisite *prereq = area1->GetPrerequisiteVector()[i];
		if (prereq == nullptr || prereq->m_isDisabled ||
			!prereq->PassesTriggerFilters(actor)) {
			continue;
		}
		
		if (prereq->IsTask(CFuncNavPrerequisite::TaskType::WAIT)) {
			return EventDesiredResult<CTFBot>::SuspendFor(new CTFBotNavEntMoveTo(prereq,
				"Prerequisite commands me to move to an entity"));
		}
		
		if (prereq->IsTask(CFuncNavPrerequisite::TaskType::MOVE_TO)) {
			return EventDesiredResult<CTFBot>::SuspendFor(new CTFBotNavEntWait(prereq,
				"Prerequisite commands me to wait"));
		}
	}
	
	return EventDesiredResult<CTFBot>::Continue();
}
