/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/tf_bot_body.cpp
 */


CTFBotBody::CTFBotBody(INextBot *nextbot)
	: PlayerBody(nextbot)
{
}

CTFBotBody::~CTFBotBody()
{
}


float CTFBotBody::GetHeadAimTrackingInterval() const
{
	CTFBot *bot = static_cast<CTFBot *>(this->GetBot());
	
	if (TFGameRules()->IsMannVsMachineMode() &&
		bot->IsPlayerClass(TF_CLASS_SPY)) {
		return 0.25f;
	}
	
	switch (bot->m_iSkill) {
	case CTFBot::DifficultyType::EASY:
		return 1.00f;
	case CTFBot::DifficultyType::NORMAL:
		return 0.25f;
	case CTFBot::DifficultyType::HARD:
		return 0.10f;
	case CTFBot::DifficultyType::EXPERT:
		return 0.05f;
	default:
		return 0.00f;
	}
}
