/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/squad/tf_bot_escort_squad_leader.cpp
 * used in MvM: TODO
 * 
 * initial contained action of CTFBotScenarioMonitor (for non-medic squad members)
 */


ConVar tf_bot_squad_escort_range("tf_bot_squad_escort_range", "500", FCVAR_CHEAT);
ConVar tf_bot_formation_debug("tf_bot_formation_debug", "0", FCVAR_CHEAT);


CTFBotEscortSquadLeader::CTFBotEscortSquadLeader(Action<CTFBot *> *done_action)
{
	this->m_DoneAction = done_action;
}

CTFBotEscortSquadLeader::~CTFBotEscortSquadLeader()
{
}


const char *CTFBotEscortSquadLeader::GetName() const
{
	return "EscortSquadLeader";
}


ActionResult<CTFBot> CTFBotEscortSquadLeader::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	/* BUG: doesn't set PathFollower's min lookahead distance */
	
	this->m_vecLeaderGoalDirection = vec3_origin;
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotEscortSquadLeader::Update(CTFBot *actor, float dt)
{
	if (dt < 0.0f) {
		return ActionResult<CTFBot>::Continue();
	}
	
	CTFBotSquad *squad = actor->m_Squad;
	if (squad == nullptr) {
		if (this->m_DoneAction != nullptr) {
			return ActionResult<CTFBot>::ChangeTo(this->m_DoneAction, "Not in a Squad");
		} else {
			return ActionResult<CTFBot>::Done("Not in a Squad");
		}
	}
	
	// TODO: figure out what the byte @ INextBot+0x10 is supposed to be
	// this function sets it to true right here
	// but as far as I can tell, nothing in INextBot actually accesses it
	// and none of the CTFBot code besides this seem to touch it (CTFBot+0x29e0)
	
	CTFBot *leader = squad->GetLeader();
	if (leader == nullptr || !leader->IsAlive()) {
		actor->LeaveSquad();
		
		if (this->m_DoneAction != nullptr) {
			return ActionResult<CTFBot>::ChangeTo(this->m_DoneAction, "Squad leader is dead");
		} else {
			return ActionResult<CTFBot>::Done("Squad leader is dead");
		}
	}
	
	if (TFGameRules() != nullptr && TFGameRules()->IsMannVsMachineMode() &&
		actor == leader) {
		if ((actor->m_nBotAttrs & CTFBot::AttributeType::AGGRESSIVE) != 0) {
			return ActionResult<CTFBot>::ChangeTo(new CTFBotPushToCapturePoint(new CTFBotFetchFlag(false)),
				"I'm now the squad leader! Going for the flag!");
		} else {
			return ActionResult<CTFBot>::ChangeTo(new CTFBotFetchFlag(false),
				"I'm now the squad leader! Going for the flag!");
		}
	}
	
	CTFWeaponBase *weapon = actor->m_Shared.GetActiveTFWeapon();
	if (weapon != nullptr && weapon->IsMeleeWeapon()) {
		if (actor->IsRangeLessThan(leader, tf_bot_squad_escort_range.GetFloat())) {
			if (actor->IsLineOfSightClear(leader, IGNORE_NOTHING)) {
				ActionResult<CTFBot> result = this->m_MeleeAttack->Update(actor, dt);
				if (result.transition == ActionTransition::CONTINUE) {
					return ActionResult<CTFBot>::Continue();
				}
			}
		}
	}
	
	CUtlVector<CTFBot *> members;
	squad->CollectMembers(&members);
	
	CUtlVector<CTFBot *> non_medics;
	FOR_EACH_VEC(members, i) {
		CTFBot *member = members[i];
		if (!member->IsPlayerClass(TF_CLASS_MEDIC)) {
			non_medics.AddToTail(member);
		}
	}
	
	PathFollower *leader_path = leader->GetCurrentPath();
	if (leader_path == nullptr || leader_path->GetCurrentGoal() == nullptr) {
		this->m_bIsInFormation = false;
		actor->m_flFormationError = 0.0f;
		
		return ActionResult<CTFBot>::Continue();
	}
	
	const Path::Segment *leader_goal = leader_path->GetCurrentGoal();
	Vector vec_to_goal = (leader_goal->m_vecStart - leader->GetAbsOrigin());
	if (vec_to_goal.IsLengthLessThan(25.0f)) {
		const Path::Segment *next_seg = leader_path->NextSegment(leader_goal);
		if (next_seg != nullptr) {
			vec_to_goal = (next_seg->m_vecStart - leader->GetAbsOrigin());
		}
	}
	
	vec_to_goal.NormalizeInPlace();
	if (!this->m_vecLeaderGoalDirection.IsZero()) {
		float yaw_this = UTIL_VecToYaw(vec_to_goal);
		float yaw_prev = UTIL_VecToYaw(this->m_vecLeaderGoalDirection);
		
		float yaw_diff = AngleDiff(yaw_this, yaw_prev);
		float yaw_diff_max = dt * 30.0f;
		
		float yaw_next = yaw_prev +
			Clamp(yaw_diff, -yaw_diff_max, yaw_diff_max);
		
		FastSinCos(RAD2DEG(yaw_next), &this->m_vecLeaderGoalDirection.y,
			&this->m_vecLeaderGoalDirection.x);
		this->m_vecLeaderGoalDirection.z = 0.0f;
	} else {
		this->m_vecLeaderGoalDirection = vec_to_goal;
	}
	
	float formation_size = Max(squad->m_flFormationSize, 0.0f);
	
	int idx;
	for (idx = 0; idx < squad->GetMemberCount(); ++idx) {
		if (actor->IsSelf(non_medics[idx])) {
			break;
		}
	}
	
	int my_idx = idx - 1;
	float angle = (M_PI / 6.0f) * (idx / 2);
	if (my_idx % 2 == 0) {
		angle = -angle;
	}
	
	float a_sin, a_cos;
	FastSinCos(angle, &a_sin, &a_cos);
	
	Vector2D goal_dir = this->m_vecLeaderGoalDirection.AsVector2D();
	Vector2D goal_sin = goal_dir * a_sin;
	Vector2D goal_cos = goal_dir * a_cos;
	Vector goal_rot = {
		.x = goal_cos.x - goal_sin.y,
		.y = goal_sin.x - goal_cos.y,
		.z = vec3_origin,
	};
	
	Vector ideal_pos = leader->GetAbsOrigin() + (formation_size * goal_rot);
	
	trace_t trace;
	CTraceFilterIgnoreTeammates filter(actor, COLLISION_GROUP_NONE,
		actor->GetTeamNumber());
	UTIL_TraceLine(leader->GetAbsOrigin(), ideal_pos - leader->GetAbsOrigin(),
		MASK_PLAYERSOLID, &filter, &trace);
	
	if (trace.DidHitWorld()) {
		float scale = 0.6f * actor->GetBodyInterface()->GetHullWidth();
		
		ideal_pos = (scale * trace.plane.normal) + trace.endpos;
		ideal_pos.z -= 35.5f;
	}
	
	if (tf_bot_formation_debug.GetBool()) {
		NDebugOverlay::Circle(ideal_pos, 16.0f, RGBA_GREEN, true, 0.1f);
		
		CFmtStrN<256> str_idx;
		NDebugOverlay::Text(ideal_pos, str_idx.sprintf("%d", self_idx),
			false, 0.1f);
	}
	
	Vector vec_error = ideal_pos - actor->GetAbsOrigin();
	float error = vec_error.Length2D();
	actor->m_flFormationError = Min(error, 100.0f) * (1.0f / 100.0f);
	
	if (error < 50.0f) {
		Vector vec_fix = vec_error * goal_rot;
		if (vec_fix.Length() == 0.0f) {
			actor->m_flFormationError = 0.0f;
		} else {
			actor->GetLocomotionInterface()->Approach(ideal_pos, 1.0f);
			return ActionResult<CTFBot>::Continue();
		}
	}
	
	if (this->m_ctRecomputePath.IsElapsed()) {
		this->m_ctRecomputePath.Start(RandomFloat(0.1f, 0.2f));
		actor->m_bIsInFormation = false;
		
		CTFBotPathCost cost_func(actor, FASTEST_ROUTE);
		if (!this->m_PathFollower.Compute<CTFBotPathCost>(actor,
			ideal_pos, cost_func, 0.0f, true)) {
			actor->m_bIsInFormation = true;
		}
		
		if (this->m_PathFollower.GetLength() > 750.0f) {
			actor->m_bIsInFormation = true;
		}
	}
	
	this->m_PathFollower.Update(actor);
	return ActionResult<CTFBot>::Continue();
}

void CTFBotEscortSquadLeader::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
}


CTFBotWaitForOutOfPositionSquadMember::CTFBotWaitForOutOfPositionSquadMember()
{
}

CTFBotWaitForOutOfPositionSquadMember::~CTFBotWaitForOutOfPositionSquadMember()
{
}


const char *CTFBotWaitForOutOfPositionSquadMember::GetName() const
{
	return "WaitForOutOfPositionSquadMember";
}


ActionResult<CTFBot> CTFBotWaitForOutOfPositionSquadMember::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_ctTimeout.Start(2.0f);
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotWaitForOutOfPositionSquadMember::Update(CTFBot *actor, float dt)
{
	if (this->m_ctTimeout.IsElapsed()) {
		return ActionResult<CTFBot>::Done("Timeout");
	}
	
	CTFBotSquad *squad = actor->m_Squad;
	if (squad == nullptr || actor != squad->GetLeader()) {
		return ActionResult<CTFBot>::Done("No squad");
	}
	
	if (squad->IsInFormation()) {
		return ActionResult<CTFBot>::Done("Everyone is in formation. Moving on.");
	} else {
		return ActionResult<CTFBot>::Continue();
	}
}
