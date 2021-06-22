/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_approach_object.cpp
 * used in MvM: TODO
 * 
 * completely unused
 */


CTFBotApproachObject::CTFBotApproachObject(CBaseEntity *object, float dist)
{
	this->m_hObject = object;
	this->m_flDist  = dist;
}

CTFBotApproachObject::~CTFBotApproachObject()
{
}


const char *CTFBotApproachObject::GetName() const
{
	return "ApproachObject";
}


ActionResult<CTFBot> CTFBotApproachObject::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotApproachObject::Update(CTFBot *actor, float dt)
{
	if (this->m_hObject == nullptr) {
		return ActionResult<CTFBot>::Done("Object is NULL");
	}
	
	if (this->m_hObject->IsEffectActive(EF_NODRAW)) {
		return ActionResult<CTFBot>::Done("Object is NODRAW");
	}
	
	if (actor->GetLocomotionInterface()->GetGround() == this->m_hObject) {
		return ActionResult<CTFBot>::Done("I'm standing on the object");
	}
	
	if ((actor->GetAbsOrigin() - this->m_hObject->GetAbsOrigin()).LengthSqr() < Square(this->m_flDist)) {
		return ActionResult<CTFBot>::Done("Reached object");
	}
	
	if (this->m_ctRecomputePath.IsElapsed()) {
		this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
		
		CTFBotPathCost cost_func(actor, FASTEST_ROUTE);
		this->m_PathFollower.Compute(actor, this->m_hObject->GetAbsOrigin(), cost_func, 0.0f, true);
	}
	
	this->m_PathFollower.Update(actor);
	
	return ActionResult<CTFBot>::Continue();
}
