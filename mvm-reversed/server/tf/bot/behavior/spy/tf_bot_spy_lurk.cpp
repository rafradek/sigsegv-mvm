/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/spy/tf_bot_spy_lurk.cpp
 * used in MvM: TODO
 */


CTFBotSpyLurk::CTFBotSpyLurk()
{
}

CTFBotSpyLurk::~CTFBotSpyLurk()
{
}


const char *CTFBotSpyLurk::GetName() const
{
	return "SpyLurk";
}


ActionResult<CTFBot> CTFBotSpyLurk::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	if (!actor->m_Shared.IsStealthed()) {
		actor->PressAltFireButton();
	}
	
	actor->DisguiseAsMemberOfEnemyTeam();
	
	this->m_ctPatience.Start(RandomFloat(3.0f, 5.0f));
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSpyLurk::Update(CTFBot *actor, float dt)
{
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
	if (threat != nullptr && threat->GetEntity() != nullptr) {
		auto obj = dynamic_cast<CBaseObject *>(threat->GetEntity());
		if (obj != nullptr && !obj->HasSapper() && actor->IsEnemy(obj)) {
			return ActionResult<CTFBot>::SuspendFor(new CTFBotSpySap(obj),
				"Sapping an enemy object");
		}
	}
	
	if (actor->m_hTargetSentry != nullptr && !actor->m_hTargetSentry->HasSapper()) {
		return ActionResult<CTFBot>::SuspendFor(new CTFBotSpySap(actor->m_hTargetSentry),
			"Sapping a Sentry");
	}
	
	if (this->m_ctPatience.IsElapsed()) {
		return ActionResult<CTFBot>::Done("Lost patience with my hiding spot");
	}
	
	if (actor->GetLastKnownArea() != nullptr && threat != nullptr && threat->GetTimeSinceLastKnown() < 3.0f) {
		CTFPlayer *victim = ToTFPlayer(threat->GetEntity());
		if (victim != nullptr && !victim->IsLookingTowards(actor, 0.9f)) {
			return ActionResult<CTFBot>::ChangeTo(new CTFBotSpyAttack(victim),
				"Going after a backstab victim");
		}
	}
	
	return ActionResult<CTFBot>::Continue();
}


QueryResponse CTFBotSpyLurk::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	return QueryResponse::NO;
}
