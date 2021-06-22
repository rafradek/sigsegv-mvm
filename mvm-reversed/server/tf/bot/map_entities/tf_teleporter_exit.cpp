/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/map_entities/tf_teleporter_exit.cpp
 */


CTFBotHintTeleporterExit::CTFBotHintTeleporterExit()
{
}

CTFBotHintTeleporterExit::~CTFBotHintTeleporterExit()
{
}


CBaseTFBotHintEntity::HintType CTFBotHintTeleporterExit::GetHintType() const
{
	return CBaseTFBotHintEntity::HintType::TELEPORTER_EXIT;
}
