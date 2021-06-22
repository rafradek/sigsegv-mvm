/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/map_entities/tf_sentrygun.cpp
 */


CTFBotHintSentrygun::CTFBotHintSentrygun()
{
	// m_isSticky = false
	// 370 = 0
}

CTFBotHintSentrygun::~CTFBotHintSentrygun()
{
}


CBaseTFBotHintEntity::HintType CTFBotHintSentrygun::GetHintType() const
{
	return CBaseTFBotHintEntity::HintType::SENTRY_GUN;
}


bool CTFBotHintSentrygun::IsAvailableForSelection(CTFPlayer *player) const
{
	if (this->[[handle@0x38c]] != nullptr && this->[[handle@0x38c]]->IsPlayerClass(TF_CLASS_ENGINEER)) {
		return false;
	} else if (this->m_bDisabled || this->[[dword@0x370]] != 0) {
		return false;
	} else {
		return this->InSameTeam(player);
	}
}


void CTFBotHintSentrygun::OnSentryGunDestroyed(CBaseEntity *ent)
{
	this->m_OnSentryGunDestroyed->FireOutput(ent, ent);
}
