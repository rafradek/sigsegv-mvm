/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/engineer/tf_bot_engineer_build.cpp
 * used in MvM: TODO
 */


ConVar tf_raid_engineer_infinte_metal("tf_raid_engineer_infinte_metal", "1", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY);


CTFBotEngineerBuild::CTFBotEngineerBuild(/* TODO */)
{
	// TODO
}

CTFBotEngineerBuild::~CTFBotEngineerBuild()
{
}


const char *CTFBotEngineerBuild::GetName() const
{
	return "EngineerBuild";
}


ActionResult<CTFBot> CTFBotEngineerBuild::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotEngineerBuild::Update(CTFBot *actor, float dt)
{
	// TODO
}

ActionResult<CTFBot> CTFBotEngineerBuild::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	return ActionResult<CTFBot>::Continue();
}


Action<CTFBot> *CTFBotEngineerBuild::InitialContainedAction(CTFBot *actor)
{
	// TODO
}


EventDesiredResult<CTFBot> CTFBotEngineerBuild::OnTerritoryLost(CTFBot *actor, int i1)
{
	return Continue();
}


QueryResponse CTFBotEngineerBuild::ShouldHurry(const INextBot *nextbot) const
{
	// TODO
}

QueryResponse CTFBotEngineerBuild::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	// TODO
}
