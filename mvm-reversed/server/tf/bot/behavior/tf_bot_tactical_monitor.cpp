/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_tactical_monitor.cpp
 * used in MvM: TODO
 * 
 * initial contained action of CTFBotMainAction
 */


ConVar tf_bot_force_jump("tf_bot_force_jump", "0", FCVAR_CHEAT,
	"Force bots to continuously jump");


CTFBotTacticalMonitor::CTFBotTacticalMonitor()
{
}

CTFBotTacticalMonitor::~CTFBotTacticalMonitor()
{
}


const char *CTFBotTacticalMonitor::GetName() const
{
	return "TacticalMonitor";
}


ActionResult<CTFBot> CTFBotTacticalMonitor::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotTacticalMonitor::Update(CTFBot *actor, float dt)
{
	if (TFGameRules()->State_Get() == GR_STATE_TEAM_WIN) {
		if (TFGameRules()->GetWinningTeam() == actor->GetTeamNumber()) {
			return ActionResult<CTFBot>::SuspendFor(new CTFBotSeekAndDestroy(),
				"Get the losers!");
		} else {
			if (actor->GetVisionInterface()->GetPrimaryKnownThreat(true) != nullptr) {
				return ActionResult<CTFBot>::SuspendFor(new CTFBotRetreatToCover(-1.0f),
					"Run away from threat!");
			} else {
				actor->PressCrouchButton();
				return ActionResult<CTFBot>::Continue();
			}
		}
	}
	
	if (tf_bot_force_jump.GetBool() && !actor->GetLocomotionInterface()->IsClimbingOrJumping()) {
		actor->GetLocomotionInterface()->Jump();
	}
	
	if (TFGameRules()->State_Get() == GR_STATE_PREROUND) {
		actor->GetLocomotionInterface()->ClearStuckStatus("In preround");
	}
	
	Action<CTFBot> *action = actor->OpportunisticallyUseWeaponAbilities();
	if (action != nullptr) {
		return ActionResult<CTFBot>::SuspendFor(action, "Opportunistically using buff item");
	}
	
	if (TFGameRules()->InSetup()) {
		// TODO: taunting at humans stuff
	}
	
	QueryResponse retreat = actor->GetIntentionInterface()->ShouldRetreat(actor);
	if (!TFGameRules()->IsMannVsMachineMode) {
		if (retreat == QueryResponse::YES) {
			return ActionResult<CTFBot>::SuspendFor(new CTFBotRetreatToCover(-1.0f), "Backing off");
		}
		
		if (retreat != QueryResponse::NO) {
			/* BUG: should use IsInvulnerable, to cover other uber conds */
			if (!actor->m_Shared.InCond(TF_COND_INVULNERABLE) &&
				(actor->m_iSkill == CTFBot::DifficultyType::HARD || actor->m_iSkill == CTFBot::DifficultyType::EXPERT)) {
				// TODO: out-of-ammo RetreatToCover
			}
		}
	}
	
	QueryResponse hurry = actor->GetIntentionInterface()->ShouldHurry(actor);
	if ((TFGameRules()->IsMannVsMachineMode() && actor->HasTheFlag()) ||
		hurry == QueryResponse::YES || /* TODO: countdown timer @ 0x34 */) {
		// TODO
	} else {
		/* TODO: name */
		this->m_ct34.Start(RandomFloat(0.3f, 0.5f));
		
		bool low_health = false;
		
		if ((actor->GetTimeSinceWeaponFired() < 2.0f || actor->IsPlayerClass(TF_CLASS_SNIPER)) &&
			(float)actor->GetHealth() / (float)actor->GetMaxHealth() < tf_bot_health_critical_ratio.GetFloat()) {
			low_health = true;
		} else if (actor->m_Shared.InCond(TF_COND_BURNING) ||
			(float)actor->GetHealth() / (float)actor->GetMaxHealth() < tf_bot_health_ok_ratio.GetFloat()) {
			low_health = true;
		}
		
		if (low_health && CTFBotGetHealth::IsPossible(actor)) {
			return ActionResult<CTFBot>::SuspendFor(new CTFBotGetHealth(), "Grabbing nearby health");
		}
		
		if (actor->IsAmmoLow() && CTFBotGetAmmo::IsPossible(actor)) {
			return ActionResult<CTFBot>::SuspendFor(new CTFBotGetAmmo(), "Grabbing nearby ammo");
		}
		
		if (!TFGameRules()->IsMannVsMachineMode()) {
			// TODO: logic for CTFBotDestroyEnemySentry
		}
	}
	// TODO: figure out the control flow here, it's rather complex
	// TODO: opportunistically use teleporter
	// (suspend for CTFBotUseTeleporter)
	
	// TODO
}


Action<CTFBot> *CTFBotTacticalMonitor::InitialContainedAction(CTFBot *actor)
{
	return new CTFBotScenarioMonitor();
}


EventDesiredResult<CTFBot> CTFBotTacticalMonitor::OnOtherKilled(CTFBot *actor, CBaseCombatCharacter *who, const CTakeDamageInfo& info)
{
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotTacticalMonitor::OnNavAreaChanged(CTFBot *actor, CNavArea *area1, CNavArea *area2)
{
	if (area1 == nullptr || (actor->m_nBotAttrs & CTFBot::AttributeType::AGGRESSIVE) != 0) {
		return EventDesiredResult<CTFBot>::Continue();
	}
	
	FOR_EACH_VEC(area1->m_prerequisiteVector, i) {
		CFuncNavPrerequisite *prereq = area1->m_prerequisiteVector[i];
		if (prereq == nullptr) continue;
		
		/* TODO: enum for task types */
		if (prereq->IsTask(3)) {
			return EventDesiredResult<CTFBot>::SuspendFor(new CTFBotNavEntWait(prereq),
				"Prerequisite commands me to wait", ResultSeverity::MEDIUM);
		}
		
		if (prereq->IsTask(2)) {
			return EventDesiredResult<CTFBot>::SuspendFor(new CTFBotNavEntMoveTo(prereq),
				"Prerequisite commands me to move to an entity", ResultSeverity::MEDIUM);
		}
	}
	
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotTacticalMonitor::OnCommandString(CTFBot *actor, const char *cmd)
{
	if (FStrEq(cmd, "goto action point")) {
		return EventDesiredResult<CTFBot>::SuspendFor(new CTFGotoActionPoint(),
			"Received command to go to action point", ResultSeverity::MEDIUM);
	}
	
	if (FStrEq(cmd, "despawn")) {
		return EventDesiredResult<CTFBot>::SuspendFor(new CTFDespawn(),
			"Received command to go to de-spawn", ResultSeverity::CRITICAL);
	}
	
	if (FStrEq(cmd, "taunt")) {
		return EventDesiredResult<CTFBot>::SuspendFor(new CTFBotTaunt(),
			"Received command to taunt", ResultSeverity::LOW);
	}
	
	if (FStrEq(cmd, "cloak")) {
		// TODO
	}
	
	if (FStrEq(cmd, "uncloak")) {
		// TODO
	}
	
	if (FStrEq(cmd, "disguise")) {
		// TODO
	}
	
	if (FStrEq(cmd, "build sentry at nearest sentry hint")) {
		// TODO
	}
	
	if (FStrEq(cmd, "attack sentry at next action point")) {
		// TODO
	}
	
	return EventDesiredResult<CTFBot>::Continue();
}


void CTFBotTacticalMonitor::AvoidBumpingEnemies(CTFBot *actor)
{
	// TODO
}

CObjectTeleporter *CTFBotTacticalMonitor::FindNearbyTeleporter(CTFBot *actor)
{
	// TODO
}

void CTFBotTacticalMonitor::MonitorArmedStickybombs(CTFBot *actor)
{
	// TODO
}

bool CTFBotTacticalMonitor::ShouldOpportunisticallyTeleport(CTFBot *actor) const
{
	if (actor->IsPlayerClass(TF_CLASS_ENGINEER)) {
		return (actor->GetObjectOfType(OBJ_TELEPORTER, 0) != nullptr);
	} else {
		return !actor->IsPlayerClass(TF_CLASS_MEDIC);
	}
}
