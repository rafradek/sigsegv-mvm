/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/missions/tf_bot_mission_destroy_sentries.cpp
 * used in MvM: TODO
 */


CTFBotMissionDestroySentries::CTFBotMissionDestroySentries(CObjectSentrygun *sentry)
{
	if (sentry != nullptr) {
		this->m_hSentry = sentry;
	}
}

CTFBotMissionDestroySentries::~CTFBotMissionDestroySentries()
{
}


const char *CTFBotMissionDestroySentries::GetName() const
{
	return "MissionDestroySentries";
}


ActionResult<CTFBot> CTFBotMissionDestroySentries::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	if (actor->IsPlayerClass(TF_CLASS_MEDIC)) {
		return ActionResult<CTFBot>::ChangeTo(new CTFBotMedicHeal(),
			"My job is to heal/uber the others in the mission");
	}
	
	actor->m_nBotAttrs |= CTFBot::AttributeType::IGNOREENEMIES;
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotMissionDestroySentries::Update(CTFBot *actor, float dt)
{
	// TODO
}

void CTFBotMissionDestroySentries::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
	/* BUG: doesn't save/restore old flag value */
	actor->m_nBotAttrs &= ~CTFBot::AttributeType::IGNOREENEMIES;
}


CObjectSentrygun *CTFBotMissionDestroySentries::SelectSentryTarget(CTFBot *actor)
{
	return nullptr;
}
