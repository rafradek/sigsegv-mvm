/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_use_teleporter.cpp
 * used in MvM: TODO
 * 
 * SuspendFor from CTFBotTacticalMonitor::Update
 * SuspendFor from CTFBotMedicHeal::Update
 */


CTFBotUseTeleporter::CTFBotUseTeleporter(CObjectTeleporter *teleporter, UseHowType how)
{
	this->m_hTele = teleporter;
	
	// 481c = false
	
	this->m_How = how;
}

CTFBotUseTeleporter::~CTFBotUseTeleporter()
{
}


const char *CTFBotUseTeleporter::GetName() const
{
	return "UseTeleporter";
}


ActionResult<CTFBot> CTFBotUseTeleporter::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotUseTeleporter::Update(CTFBot *actor, float dt)
{
	if (this->m_hTele == nullptr) {
		return ActionResult<CTFBot>::Done("Teleporter is gone");
	}
	
	CObjectTeleporter *exit = this->m_hTele->GetMatchingTeleporter();
	if (exit == nullptr) {
		return ActionResult<CTFBot>::Done("Missing teleporter exit");
	}
	
	// TODO
	
	
}


bool CTFBotUseTeleporter::IsTeleporterAvailable() const
{
	if (this->m_hTele == nullptr || !this->m_hTele->IsReady()) {
		return false;
	}
	
	return (this->m_hTele->GetState() == TELEPORTER_STATE_READY);
}
