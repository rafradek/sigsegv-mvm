/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/engineer/tf_bot_engineer_build_teleport_exit.cpp
 * used in MvM: TODO
 */


CTFBotEngineerBuildTeleportExit::CTFBotEngineerBuildTeleportExit()
{
	// 4808 = false
}

CTFBotEngineerBuildTeleportExit::CTFBotEngineerBuildTeleportExit(const Vector& v1, float f1)
{
	// TODO: names for parameters
	
	// 4808 = true
	// 480c = v1
	// 4818 = f1
}

CTFBotEngineerBuildTeleportExit::~CTFBotEngineerBuildTeleportExit()
{
}


const char *CTFBotEngineerBuildTeleportExit::GetName() const
{
	return "EngineerBuildTeleportExit";
}


ActionResult<CTFBot> CTFBotEngineerBuildTeleportExit::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	if (!/* 4808 */) {
		// 480c = actor->GetAbsOrigin()
	}
	
	// 481c .Start(3.1f)
	
	this->m_PathFollower.Invalidate();
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotEngineerBuildTeleportExit::Update(CTFBot *actor, float dt)
{
	// TODO
}

ActionResult<CTFBot> CTFBotEngineerBuildTeleportExit::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	// 481c .Reset()
	this->m_PathFollower.Invalidate();
	
	return ActionResult<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotEngineerBuildTeleportExit::OnStuck(CTFBot *actor)
{
	this->m_PathFollower.Invalidate();
	
	return EventDesiredResult<CTFBot>::Continue();
}
