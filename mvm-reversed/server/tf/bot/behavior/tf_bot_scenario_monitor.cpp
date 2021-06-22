/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_scenario_monitor.cpp
 * used in MvM: TODO
 * 
 * initial contained action of CTFBotTacticalMonitor
 */


ConVar tf_bot_fetch_lost_flag_time("tf_bot_fetch_lost_flag_time", "10", FCVAR_CHEAT,
	"How long busy TFBots will ignore the dropped flag before they give up what they are doing and go after it");
ConVar tf_bot_flag_kill_on_touch("tf_bot_flag_kill_on_touch", "0", FCVAR_CHEAT,
	"If nonzero, any bot that picks up the flag dies. For testing.");


CTFBotScenarioMonitor::CTFBotScenarioMonitor()
{
}

CTFBotScenarioMonitor::~CTFBotScenarioMonitor()
{
}


const char *CTFBotScenarioMonitor::GetName() const
{
	return "ScenarioMonitor";
}


ActionResult<CTFBot> CTFBotScenarioMonitor::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_ctFetchFlagInitial.Start(20.0f);
	this->m_ctFetchFlag.Invalidate();
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotScenarioMonitor::Update(CTFBot *actor, float dt)
{
	if (actor->HasTheFlag(false, false)) {
		if (tf_bot_flag_kill_on_touch.GetBool()) {
			actor->CommitSuicide(false, true);
			return ActionResult<CTFBot>::Done("Flag kill");
		}
		
		// TODO: parameters for CTFBotDeliverFlag ctor
		return ActionResult<CTFBot>::SuspendFor(new CTFBotDeliverFlag(/* TODO */),
			"I've picked up the flag! Running it in...");
	}
	
	if (actor->m_nMission != CTFBot::MissionType::NONE) {
		return ActionResult<CTFBot>::Continue();
	}
	
	if (this->m_ctFetchFlagInitial.IsElapsed() &&
		actor->IsAllowedToPickUpFlag()) {
		CCaptureFlag *flag = actor->GetFlagToFetch();
		if (flag == nullptr) {
			return ActionResult<CTFBot>::Continue();
		}
		
		CTFPlayer *owner = ToTFPlayer(flag->GetOwnerEntity());
		if (owner != nullptr) {
			this->m_ctFetchFlag.Invalidate();
		} else {
			if (this->m_ctFetchFlag.HasStarted()) {
				if (this->m_ctFetchFlag.IsElapsed()) {
					this->m_ctFetchFlag.Invalidate();
					
					if (actor->MedicGetHealTarget() == nullptr) {
						return ActionResult<CTFBot>::SuspendFor(new CTFBotFetchFlag(true),
							"Fetching lost flag...");
					}
				}
			} else {
				this->m_ctFetchFlag.Start(tf_bot_fetch_lost_flag_time.GetFloat());
			}
		}
	}
	
	return ActionResult<CTFBot>::Continue();
}


Action<CTFBot> *CTFBotScenarioMonitor::InitialContainedAction(CTFBot *actor)
{
	CTFBot *squad = actor->m_Squad;
	if (squad == nullptr || actor == squad->GetLeader()) {
		return this->DesiredScenarioAndClassAction(actor);
	}
	
	if (actor->IsPlayerClass(TF_CLASS_MEDIC)) {
		return new CTFBotMedicHeal();
	} else {
		return new CTFBotEscortSquadLeader(this->DesiredScenarioAndClassAction());
	}
}


Action<CTFBot> *CTFBotScenarioMonitor::DesiredScenarioAndClassAction(CTFBot *actor)
{
	CTFBot::MissionType mission = actor->m_nMission;
	
	if (mission == CTFBot::MissionType::DESTROY_SENTRIES) {
		return new CTFBotMissionSuicideBomber();
	} else if (mission == CTFBot::MissionType::SNIPER) {
		return new CTFBotSniperLurk(/* TODO */);
	}
	
	if (TFGameRules()->IsMannVsMachineMode()) {
		if (actor->IsPlayerClass(TF_CLASS_SPY)) {
			return new CTFBotSpyLeaveSpawnRoom();
		}
		
		if (actor->IsPlayerClass(TF_CLASS_MEDIC)) {
			bool has_medic = false;
			
			int num_healers = actor->m_Shared.m_nNumHealers;
			for (int i = 0; i < num_healers; ++i) {
				CTFPlayer *healer = ToTFPlayer(actor->m_Shared.GetHealerByIndex(i));
				if (healer != nullptr) {
					has_medic = true;
					break;
				}
			}
			
			if (!has_medic) {
				return new CTFBotMedicHeal();
			}
		}
		
		if (actor->IsPlayerClass(TF_CLASS_ENGINEER)) {
			return new CTFBotMvMEngineerIdle();
		}
		
		if ((actor->m_nBotAttrs & CTFBot::AttributeType::AGGRESSIVE) != 0) {
			return new CTFBotPushToCapturePoint(new CTFBotFetchFlag(false));
		} else {
			return new CTFBotFetchFlag(false);
		}
	}
	
	if (actor->IsPlayerClass(TF_CLASS_SPY)) {
		return new CTFBotSpyInfiltrate(/* TODO */);
	}
	
	if (!TheTFBots().IsMeleeOnly()) {
		if (actor->IsPlayerClass(TF_CLASS_SNIPER)) {
			return new CTFBotSniperLurk(/* TODO */);
		}
		
		if (actor->IsPlayerClass(TF_CLASS_MEDIC)) {
			return new CTFBotMedicHeal();
		}
		
		if (actor->IsPlayerClass(TF_CLASS_ENGINEER)) {
			return new CTFBotEngineerBuild(/* TODO */);
		}
	}
	
	if (actor->GetFlagToFetch() != nullptr) {
		return new CTFBotFetchFlag(false);
	}
	
	if (TFGameRules()->GetGameType() == TF_GAMETYPE_ESCORT) {
		if (actor->GetTeamNumber() == TF_TEAM_BLUE) {
			return new CTFBotPayloadPush(/* TODO */);
		} else if (actor->GetTeamNumber() == TF_TEAM_RED) {
			return new CTFBotPayloadGuard(/* TODO */);
		} else {
			return nullptr;
		}
	}
	
	if (TFGameRules()->GetGameType() == TF_GAMETYPE_CP) {
		CUtlVector<CTeamControlPoint *> capture_points;
		TFGameRules()->CollectCapturePoints(actor, &capture_points);
		
		if (!capture_points.IsEmpty()) {
			return new CTFBotCapturePoint();
		}
		
		CUtlVector<CTeamControlPoint *> defend_points;
		TFGameRules()->CollectDefendPoints(actor, &defend_points);
		
		if (!defend_points.IsEmpty()) {
			return new CTFBotDefendPoint(/* TODO */);
		}
		
		DevMsg("%3.2f: %s: Gametype is CP, but I can't find a point to capture or defend!\n",
			gpGlobals->curtime, actor->GetDebugIdentifier());
		return new CTFBotCapturePoint();
	}
	
	return new CTFBotSeekAndDestroy(-1.0f);
}
