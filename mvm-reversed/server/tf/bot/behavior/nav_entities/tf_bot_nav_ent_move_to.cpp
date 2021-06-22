/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/nav_entities/tf_bot_nav_ent_move_to.cpp
 * used in MvM: TODO
 */


CTFBotNavEntMoveTo::CTFBotNavEntMoveTo(const CFuncNavPrerequisite *prereq)
{
	this->m_hPrereq = prereq;
	this->m_GoalArea = nullptr;
}

CTFBotNavEntMoveTo::~CTFBotNavEntMoveTo()
{
}


const char *CTFBotNavEntMoveTo::GetName() const
{
	return "NavEntMoveTo";
}


ActionResult<CTFBot> CTFBotNavEntMoveTo::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	if (this->m_hPrereq == nullptr) {
		return ActionResult<CTFBot>::Done("Prerequisite has been removed before we started");
	}
	
	this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
	
	this->m_ctWaitDuration.Invalidate();
	
	CBaseEntity *target = this->m_hPrereq->GetTaskEntity();
	if (target == nullptr) {
		return ActionResult<CTFBot>::Done("Prerequisite target entity is NULL");
	}
	
	Extent ext;
	ext.Init(target);
	
	this->m_vecGoalPos.x = ext.lo.x + RandomFloat(0.0f, ext.SizeX());
	this->m_vecGoalPos.y = ext.lo.y + RandomFloat(0.0f, ext.SizeY());
	this->m_vecGoalPos.z = ext.SizeZ() - ext.lo.z;
	
	TheNavMesh->GetSimpleGroundHeight(this->m_vecGoalPos, &this->m_vecGoalPos.z);
	this->m_GoalArea = TheNavMesh->GetNavArea(this->m_vecGoalPos);
	
	if (this->m_GoalArea == nullptr) {
		return ActionResult<CTFBot>::Done("There's no nav area for the goal position");
	}
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotNavEntMoveTo::Update(CTFBot *actor, float dt)
{
	if (this->m_hPrereq == nullptr) {
		return ActionResult<CTFBot>::Done("Prerequisite has been removed");
	}
	
	if (this->m_hPrereq->m_isDisabled) {
		return ActionResult<CTFBot>::Done("Prerequisite has been disabled");
	}
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
	if (threat != nullptr && threat->IsVisibleRecently()) {
		actor->EquipBestWeaponForThreat(threat);
	}
	
	if (!this->m_ctWaitDuration.HasStarted()) {
		if (actor->GetLastKnownArea() == this->m_GoalArea) {
			this->m_ctWaitDuration.Start(this->m_hPrereq->m_taskValue);
		} else {
			if (this->m_ctRecomputePath.IsElapsed()) {
				this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
				
				CTFBotPathCost cost_func(actor, FASTEST_ROUTE);
				this->m_PathFollower.Compute(actor, this->m_vecGoalPos, cost_func, 0.0f, true);
			}
			
			this->m_PathFollower.Update(actor);
		}
		
		return ActionResult<CTFBot>::Continue();
	}
	
	if (this->m_ctWaitDuration.IsElapsed()) {
		return ActionResult<CTFBot>::Done("Wait duration elapsed");
	}
	
	return ActionResult<CTFBot>::Continue();
}
