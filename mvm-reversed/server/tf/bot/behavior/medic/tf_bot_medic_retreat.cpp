/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/medic/tf_bot_medic_retreat.cpp
 * used in MvM: TODO
 */


CTFBotMedicRetreat::CTFBotMedicRetreat()
{
}
 
CTFBotMedicRetreat::~CTFBotMedicRetreat()
{
}


const char *CTFBotMedicRetreat::GetName() const
{
	return "MedicRetreat";
}


ActionResult<CTFBot> CTFBotMedicRetreat::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	if (actor->m_HomeArea == nullptr) {
		return ActionResult<CTFBot>::Done("No home area!");
	}
	
	CTFBotPathCost cost_func(actor, FASTEST_ROUTE);
	this->m_PathFollower.Compute(actor, actor->m_HomeArea.GetCenter(), cost_func, 0.0f, true);
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotMedicRetreat::Update(CTFBot *actor, float dt)
{
	CTFWeaponBase *weapon = actor->m_Shared.GetActiveTFWeapon();
	if (weapon != nullptr && weapon->GetWeaponID() != TF_WEAPON_SYRINGEGUN_MEDIC) {
		CBaseCombatWeapon *primary = actor->Weapon_GetSlot(0);
		if (primary != nullptr) {
			actor->Weapon_Switch(primary);
		}
	}
	
	this->m_PathFollower.Update(actor);
	
	if (this->m_ctLookForPatients.IsElapsed()) {
		this->m_ctLookForPatients.Start(RandomFloat(0.33f, 1.00f));
		
		Vector fwd;
		AngleVectors(QAngle(0.0f, RandomFloat(-180.0f, 180.0f), 0.0f), &fwd);
		
		actor->GetBodyInterface()->AimHeadTowards(actor->EyePosition() + fwd,
			IBody::LookAtPriorityType::IMPORTANT, 0.1f, nullptr, "Looking for someone to heal");
	}
	
	CUsefulHealTargetFilter filter(actor->GetTeamNumber());
	if (actor->GetVisionInterface()->GetClosestKnown(filter) != nullptr) {
		return ActionResult<CTFBot>::Done("I know of a teammate");
	}
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotMedicRetreat::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	CTFBotPathCost cost_func(actor, FASTEST_ROUTE);
	this->m_PathFollower.Compute(actor, actor->m_HomeArea.GetCenter(), cost_func, 0.0f, true);
	
	return ActionResult<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotMedicRetreat::OnMoveToSuccess(CTFBot *actor, const Path *path)
{
	/* uh... wtf is the purpose of this? */
	if (actor->GetAliveDuration() > 60.0f) {
		actor->CommitSuicide(false, true);
	}
	
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotMedicRetreat::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	CTFBotPathCost cost_func(actor, FASTEST_ROUTE);
	this->m_PathFollower.Compute(actor, actor->m_HomeArea.GetCenter(), cost_func, 0.0f, true);
	
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotMedicRetreat::OnStuck(CTFBot *actor)
{
	CTFBotPathCost cost_func(actor, FASTEST_ROUTE);
	this->m_PathFollower.Compute(actor, actor->m_HomeArea.GetCenter(), cost_func, 0.0f, true);
	
	return EventDesiredResult<CTFBot>::Continue();
}


QueryResponse CTFBotMedicRetreat::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	return QueryResponse::YES;
}


CUsefulHealTargetFilter::CUsefulHealTargetFilter(int teamnum)
	: m_iTeamNum(teamnum)
{
}


bool CUsefulHealTargetFilter::IsAllowed(CBaseEntity *ent) const
{
	if (ent == nullptr || !ent->IsPlayer() ||
		ent->GetTeamNumber() != this->m_iTeamNum) {
		return false;
	}
	
	if (ToTFPlayer(ent)->IsPlayerClass(TF_CLASS_MEDIC) ||
		ToTFPlayer(ent)->IsPlayerClass(TF_CLASS_SNIPER)) {
		return false;
	}
	
	return true;
}
