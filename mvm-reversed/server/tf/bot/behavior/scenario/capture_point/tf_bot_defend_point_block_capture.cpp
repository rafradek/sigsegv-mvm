/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/scenario/capture_point/tf_bot_defend_point_block_capture.cpp
 * used in MvM: TODO
 */


ConVar tf_bot_defend_owned_point_percent("tf_bot_defend_owned_point_percent", "0.5", FCVAR_CHEAT,
	"Stay on the contested point we own until enemy cap percent falls below this");


CTFBotDefendPointBlockCapture::CTFBotDefendPointBlockCapture()
{
}

CTFBotDefendPointBlockCapture::~CTFBotDefendPointBlockCapture()
{
}


const char *CTFBotDefendPointBlockCapture::GetName() const
{
	return "DefendPointBlockCapture";
}


ActionResult<CTFBot> CTFBotDefendPointBlockCapture::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	// TODO
}

ActionResult<CTFBot> CTFBotDefendPointBlockCapture::Update(CTFBot *actor, float dt)
{
	if (this->IsPointSafe(actor)) {
		return ActionResult<CTFBot>::Done("Point is safe again");
	}
	
	if (actor->IsPlayerClass(TF_CLASS_MEDIC)) {
		return ActionResult<CTFBot>::SuspendFor(new CTFBotMedicHeal());
	}
	
	
	
	// TODO
}

ActionResult<CTFBot> CTFBotDefendPointBlockCapture::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Invalidate();
	
	return ActionResult<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotDefendPointBlockCapture::OnMoveToSuccess(CTFBot *actor, const Path *path)
{
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotDefendPointBlockCapture::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	this->m_PathFollower.Invalidate();
	
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotDefendPointBlockCapture::OnStuck(CTFBot *actor)
{
	this->m_PathFollower.Invalidate();
	
	actor->GetLocomotionInterface()->ClearStuckStatus("");
	
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotDefendPointBlockCapture::OnTerritoryContested(CTFBot *actor, int i1)
{
	return EventDesiredResult<CTFBot>::Sustain();
}

EventDesiredResult<CTFBot> CTFBotDefendPointBlockCapture::OnTerritoryCaptured(CTFBot *actor, int i1)
{
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotDefendPointBlockCapture::OnTerritoryLost(CTFBot *actor, int i1)
{
	return EventDesiredResult<CTFBot>::Done("Lost the point", ResultSeverity::CRITICAL);
}


QueryResponse CTFBotDefendPointBlockCapture::ShouldHurry(const INextBot *nextbot) const
{
	return QueryResponse::YES;
}

QueryResponse CTFBotDefendPointBlockCapture::ShouldRetreat(const INextBot *nextbot) const
{
	return QueryResponse::NO;
}


bool CTFBotDefendPointBlockCapture::IsPointSafe(CTFBot *actor)
{
	// TODO
}
