/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/engineer/tf_bot_engineer_building.cpp
 * used in MvM: TODO
 */


ConVar tf_bot_engineer_retaliate_range("tf_bot_engineer_retaliate_range", "750", FCVAR_CHEAT,
	"If attacker who destroyed sentry is closer than this, attack. Otherwise, retreat");
ConVar tf_bot_engineer_exit_near_sentry_range("tf_bot_engineer_exit_near_sentry_range", "2500", FCVAR_CHEAT,
	"Maximum travel distance between a bot's Sentry gun and its Teleporter Exit");
ConVar tf_bot_engineer_max_sentry_travel_distance_to_point("tf_bot_engineer_max_sentry_travel_distance_to_point", "2500", FCVAR_CHEAT,
	"Maximum travel distance between a bot's Sentry gun and the currently contested point");


CTFBotEngineerBuilding::CTFBotEngineerBuilding()
{
}

CTFBotEngineerBuilding::CTFBotEngineerBuilding(CTFBotHintSentrygun *hint)
{
	if (hint != nullptr) {
		this->m_hHint = hint;
	}
}

CTFBotEngineerBuilding::~CTFBotEngineerBuilding()
{
}


const char *CTFBotEngineerBuilding::GetName() const
{
	return "EngineerBuilding";
}


ActionResult<CTFBot> CTFBotEngineerBuilding::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	// 0064 = 5
	
	// ct 4868 .Invalidate()
	
	// 4858 = 0
	// 486c = 0
	// 485c = 0
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotEngineerBuilding::Update(CTFBot *actor, float dt)
{
	CObjectSentrygun *sentry    = actor->GetObjectOfType(OBJ_SENTRYGUN, 0);
	CObjectDispenser *dispenser = actor->GetObjectOfType(OBJ_DISPENSER, 0);
	CObjectTeleporter *entrance = actor->GetObjectOfType(OBJ_TELEPORTER, 0);
	CObjectTeleporter *exit     = actor->GetObjectOfType(OBJ_TELEPORTER, 1);
	
	bool sentry_sapped = false;
	if (sentry != nullptr) {
		sentry_sapped = sentry->HasSapper();
	}
	
	bool dispenser_sapped = false;
	if (dispenser != nullptr) {
		dispenser_sapped = dispenser->HasSapper();
	}
	
	actor->m_bLookAroundForEnemies = true;
	
	if (sentry == nullptr) {
		// 485c = 0
		
		const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
		if (threat != nullptr && threat->IsVisibleRecently()) {
			actor->EquipBestWeaponForThreat(threat);
		}
		
		if (/* 4858 */ || /* 0064 */ <= 0) {
			return ActionResult<CTFBot>::ChangeTo(new CTFBotMvMEngineerMoveToBuild(),
				"Couldn't find a place to build");
		}
		
		// --0064
		
		if (this->m_hHint != nullptr) {
			return ActionResult<CTFBot>::SuspendFor(new CTFBotEngineerBuildSentryGun(this->m_hHint),
				"Building a Sentry at a hint location");
		} else {
			return ActionResult<CTFBot>::SuspendFor(new CTFBotEngineerBuildSentryGun(),
				"Building a Sentry");
		}
	}
	
	// 4858 = 1
	
	// TODO: logic flow gets slightly tricky here
}

void CTFBotEngineerBuilding::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
	actor->m_bLookAroundForEnemies = true;
}

ActionResult<CTFBot> CTFBotEngineerBuilding::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	return ActionResult<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotEngineerBuilding::OnTerritoryCaptured(CTFBot *actor, int i1)
{
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotEngineerBuilding::OnTerritoryLost(CTFBot *actor, int i1)
{
	return EventDesiredResult<CTFBot>::Continue();
}


bool CTFBotEngineerBuilding::CheckIfSentryIsOutOfPosition(CTFBot *actor) const
{
	CObjectSentrygun *sentry = actor->GetObjectOfType(OBJ_SENTRYGUN, 0);
	if (sentry == nullptr) {
		return false;
	}
	
	// CTFGameRules
	// 220 GetGameType
	// TF_GAMETYPE_ESCORT = 3
	
	// TODO
}

bool CTFBotEngineerBuilding::IsMetalSourceNearby(CTFBot *actor) const
{
	// TODO
}

void CTFBotEngineerBuilding::UpgradeAndMaintainBuildings(CTFBot *actor)
{
	CObjectSentrygun *sentry    = actor->GetObjectOfType(OBJ_SENTRYGUN, 0);
	CObjectDispenser *dispenser = actor->GetObjectOfType(OBJ_DISPENSER, 0);
	
	CBaseCombatWeapon *melee = actor->Weapon_GetSlot(2);
	if (melee != nullptr) {
		actor->Weapon_Switch(melee);
	}
	
	if (dispenser == nullptr) {
		float dist = (actor->GetAbsOrigin() - sentry->GetAbsOrigin()).Length();
		
		if (dist < 90.0f) {
			actor->PressCrouchButton();
		}
		
		if (dist > 75.0f) {
			// TODO: path stuff
		} else {
			// TODO
		}
		
		// TODO
	}
	
	// TODO
}
