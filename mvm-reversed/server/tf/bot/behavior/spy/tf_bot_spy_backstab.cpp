/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/spy/tf_bot_spy_backstab.cpp
 * used in MvM: TODO
 */


CTFBotSpyBackstab::CTFBotSpyBackstab()
{
}

CTFBotSpyBackstab::~CTFBotSpyBackstab()
{
}


const char *CTFBotSpyBackstab::GetName() const
{
	return "SpyBackStab";
}


ActionResult<CTFBot> CTFBotSpyBackstab::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSpyBackstab::Update(CTFBot *actor, float dt)
{
	return ActionResult<CTFBot>::Continue();
}


QueryResponse CTFBotSpyBackstab::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	return QueryResponse::NO;
}
