/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/engineer/tf_bot_engineer_build_dispenser.cpp
 * used in MvM: TODO
 */


CTFBotEngineerBuildDispenser::CTFBotEngineerBuildDispenser()
{
}

CTFBotEngineerBuildDispenser::~CTFBotEngineerBuildDispenser()
{
}


const char *CTFBotEngineerBuildDispenser::GetName() const
{
	return "EngineerBuildDispenser";
}


ActionResult<CTFBot> CTFBotEngineerBuildDispenser::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	// 0058 = 3
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotEngineerBuildDispenser::Update(CTFBot *actor, float dt)
{
	if (actor->GetTimeSinceLastInjury() < 1.0f) {
		return ActionResult<CTFBot>::Done("Ouch! I'm under attack");
	}
	
	CObjectSentrygun *sentry = actor->GetObjectOfType(OBJ_SENTRYGUN, 0);
	if (sentry == nullptr) {
		return ActionResult<CTFBot>::Done("No Sentry");
	}
	
	// TODO
	
	if (actor->GetObjectOfType(OBJ_DISPENSER, 0) != nullptr) {
		return ActionResult<CTFBot>::Done("Dispenser built");
	}
	
	if (/* 0058 <= 0 */) {
		return ActionResult<CTFBot>::Done("Can't find a place to build a Dispenser");
	}
	
	// TODO
}

void CTFBotEngineerBuildDispenser::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
	actor->GetBodyInterface()->ClearPendingAimReply();
}

ActionResult<CTFBot> CTFBotEngineerBuildDispenser::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Invalidate();
	// ct 0054 Invalidate()
	
	actor->GetBodyInterface()->ClearPendingAimReply();
	
	return ActionResult<CTFBot>::Continue();
}
