/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_dead.cpp
 * used in MvM: TODO
 * 
 * ChangeTo from CTFBotMainAction::Update
 */


CTFBotDead::CTFBotDead()
{
}

CTFBotDead::~CTFBotDead()
{
}


const char *CTFBotDead::GetName() const
{
	return "Dead";
}


ActionResult<CTFBot> CTFBotDead::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_itDeathEpoch.Start();
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotDead::Update(CTFBot *actor, float dt)
{
	if (actor->IsAlive()) {
		// TODO: figure out constructor args for CTFBotMainAction
		return ActionResult<CTFBot>::ChangeTo(new CTFBotMainAction(/* TODO */), "This should not happen!");
	}
	
	if (!this->m_itDeathEpoch.IsGreaterThen(5.0f)) {
		return ActionResult<CTFBot>::Continue();
	}
	
	if ((actor->m_nBotAttrs & CTFBot::AttributeType::REMOVEONDEATH) != 0) {
		// TODO: kickid stuff
		return ActionResult<CTFBot>::Continue();
	}
	
	if ((actor->m_nBotAttrs & CTFBot::AttributeType::BECOMESPECTATORONDEATH) == 0) {
		return ActionResult<CTFBot>::Continue();
	}
	
	actor->ChangeTeam(TEAM_SPECTATOR, false, true);
	return ActionResult<CTFBot>::Done();
}
