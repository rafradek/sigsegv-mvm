/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/engineer/tf_bot_engineer_build_sentrygun.cpp
 * used in MvM: TODO
 */


CTFBotEngineerBuildSentryGun::CTFBotEngineerBuildSentryGun()
{
	this->m_pHint = nullptr;
}

CTFBotEngineerBuildSentryGun::CTFBotEngineerBuildSentryGun(CTFBotHintSentrygun *hint)
{
	this->m_pHint = hint;
}

CTFBotEngineerBuildSentryGun::~CTFBotEngineerBuildSentryGun()
{
}


const char *CTFBotEngineerBuildSentryGun::GetName() const
{
	return "EngineerBuildSentryGun";
}


ActionResult<CTFBot> CTFBotEngineerBuildSentryGun::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	// 0070 = 5
	
	// ct 0040 Invalidate()
	// ct 0034 Invalidate()
	
	// 4858 = 1
	// 485c = true
	
	if (this->m_pHint != nullptr) {
		this->m_vecTarget = this->m_pHint->GetAbsOrigin();
	} else {
		this->m_vecTarget = actor->GetAbsOrigin();
	}
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotEngineerBuildSentryGun::Update(CTFBot *actor, float dt)
{
	if (actor->GetTimeSinceLastInjury() < 1.0f) {
		return ActionResult<CTFBot>::Done("Ouch! I'm under attack");
	}
	
	if (actor->GetObjectOfType(OBJ_SENTRYGUN, 0) != nullptr) {
		return ActionResult<CTFBot>::Done("Sentry built");
	}
	
	// TODO
}

ActionResult<CTFBot> CTFBotEngineerBuildSentryGun::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Invalidate();
	this->m_ctRecomputePath.Invalidate();
	
	return ActionResult<CTFBot>::Continue();
}
