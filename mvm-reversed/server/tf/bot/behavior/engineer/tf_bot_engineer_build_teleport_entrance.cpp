/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/engineer/tf_bot_engineer_build_teleport_entrance.cpp
 * used in MvM: TODO
 */


ConVar tf_bot_max_teleport_entrance_travel("tf_bot_max_teleport_entrance_travel", "1500", FCVAR_CHEAT,
	"Don't plant teleport entrances farther than this travel distance from our spawn room");
ConVar tf_bot_teleport_build_surface_normal_limit("tf_bot_teleport_build_surface_normal_limit", "0.99", FCVAR_CHEAT,
	"If the ground normal Z component is less that this value, Engineer bots won't place their entrance teleporter");


CTFBotEngineerBuildTeleportEntrance::CTFBotEngineerBuildTeleportEntrance()
{
}

CTFBotEngineerBuildTeleportEntrance::~CTFBotEngineerBuildTeleportEntrance()
{
}


const char *CTFBotEngineerBuildTeleportEntrance::GetName() const
{
	return "EngineerBuildTeleportEntrance";
}


ActionResult<CTFBot> CTFBotEngineerBuildTeleportEntrance::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotEngineerBuildTeleportEntrance::Update(CTFBot *actor, float dt)
{
	// TODO
}


EventDesiredResult<CTFBot> CTFBotEngineerBuildTeleportEntrance::OnStuck(CTFBot *actor)
{
	this->m_PathFollower.Invalidate();
	
	return EventDesiredResult<CTFBot>::Continue();
}
