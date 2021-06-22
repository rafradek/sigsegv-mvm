/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/nav_entities/tf_bot_nav_ent_destroy_entity.cpp
 * used in MvM: TODO
 */


CTFBotNavEntDestroyEntity::CTFBotNavEntDestroyEntity(const CFuncNavPrerequisite *prereq)
{
	this->m_hPrereq = prereq;
}

CTFBotNavEntDestroyEntity::~CTFBotNavEntDestroyEntity()
{
}


const char *CTFBotNavEntDestroyEntity::GetName() const
{
	return "NavEntDestroyEntity";
}


ActionResult<CTFBot> CTFBotNavEntDestroyEntity::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	// TODO
}

ActionResult<CTFBot> CTFBotNavEntDestroyEntity::Update(CTFBot *actor, float dt)
{
	// TODO
}

void CTFBotNavEntDestroyEntity::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
	// TODO
}


void CTFBotNavEntDestroyEntity::DetonateStickiesWhenSet(CTFBot *actor, CTFPipebombLauncher *launcher) const
{
	// TODO
}
