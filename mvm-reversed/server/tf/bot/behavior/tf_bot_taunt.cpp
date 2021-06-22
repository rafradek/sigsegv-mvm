/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_taunt.cpp
 * used in MvM: TODO
 * 
 * SuspendFor from CTFBotMainAction::Update               (respond to partner taunt)
 * SuspendFor from CTFBotMainAction::OnOtherKilled        (taunt victim)
 * SuspendFor from CTFBotTacticalMonitor::Update          (acknowledge humans)
 * SuspendFor from CTFBotTacticalMonitor::OnCommandString (nb_command taunt)
 * SuspendFor from CTFBotDeliverFlag::Update              (taunt for MvM bomb carrier upgrade)
 */


CTFBotTaunt::CTFBotTaunt()
{
}

CTFBotTaunt::~CTFBotTaunt()
{
}


const char *CTFBotTaunt::GetName() const
{
	return "Taunt";
}


ActionResult<CTFBot> CTFBotTaunt::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_ctWait.Start(RandomFloat(0.0f, 1.0f));
	
	this->m_bTaunting = false;
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotTaunt::Update(CTFBot *actor, float dt)
{
	if (!this->m_ctWait.IsElapsed()) {
		return ActionResult<CTFBot>::Continue();
	}
	
	if (!this->m_bTaunting) {
		actor->HandleTauntCommand(0);
		
		this->m_ctTaunt.Start(RandomFloat(3.0f, 5.0f));
		
		this->m_bTaunting = true;
		
		return ActionResult<CTFBot>::Continue();
	}
	
	if (this->m_ctTaunt.IsElapsed() && /* actor + 0x2228 == 3 */) {
		actor->EndLongTaunt();
	}
	
	if (!actor->m_Shared.InCond(TF_COND_TAUNTING)) {
		return ActionResult<CTFBot>::Done("Taunt finished");
	}
	
	return ActionResult<CTFBot>::Continue();
}
