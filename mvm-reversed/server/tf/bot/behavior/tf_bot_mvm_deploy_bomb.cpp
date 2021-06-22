/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_mvm_deploy_bomb.cpp
 * used in MvM: TODO
 * 
 * SuspendFor from CTFBotDeliverFlag::OnContact
 */


CTFBotMvMDeployBomb::CTFBotMvMDeployBomb()
{
}
 
CTFBotMvMDeployBomb::~CTFBotMvMDeployBomb()
{
}


const char *CTFBotMvMDeployBomb::GetName() const
{
	return "MvMDeployBomb";
}


ActionResult<CTFBot> CTFBotMvMDeployBomb::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	// TODO: dword actor+0x2830 = 1
	this->m_ctDelay.Start(tf_deploying_bomb_delay_time.GetFloat());
	
	// TODO: Vector @ this+0x40 = actor->GetAbsOrigin()
	
	actor->GetLocomotionInterface()->Stop();
	actor->SetAbsVelocity(Vector(0.0f, 0.0f, 0.0f));
	
	if (actor->IsMiniBoss()) {
#if 0
static CSchemaFieldHandle<CEconItemDefinition> pItemDef_GiantFlamethrower("MVM Giant Flamethrower");
#endif
		
		// TODO
	}
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotMvMDeployBomb::Update(CTFBot *actor, float dt)
{
	// TODO
}

void CTFBotMvMDeployBomb::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
	// TODO
}


EventDesiredResult<CTFBot> CTFBotMvMDeployBomb::OnContact(CTFBot *actor, CBaseEntity *ent, CGameTrace *trace)
{
	return Sustain(nullptr, ResultSeverity::CRITICAL);
}


QueryResponse CTFBotMvMDeployBomb::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	return QueryResponse::NO;
}
