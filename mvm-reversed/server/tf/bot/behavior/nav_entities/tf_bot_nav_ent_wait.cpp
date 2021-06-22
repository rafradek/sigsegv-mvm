/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/nav_entities/tf_bot_nav_ent_wait.cpp
 * used in MvM: TODO
 */


CTFBotNavEntWait::CTFBotNavEntWait(const CFuncNavPrerequisite *prereq)
{
	if (prereq != nullptr) {
		this->m_hPrereq = prereq;
	}
}

CTFBotNavEntWait::~CTFBotNavEntWait()
{
}


const char *CTFBotNavEntWait::GetName() const
{
	return "NavEntWait";
}


ActionResult<CTFBot> CTFBotNavEntWait::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	if (this->m_hPrereq == nullptr) {
		return ActionResult<CTFBot>::Done("Prerequisite has been removed before we started");
	}
	
	// TODO
}

ActionResult<CTFBot> CTFBotNavEntWait::Update(CTFBot *actor, float dt)
{
	// TODO
}
