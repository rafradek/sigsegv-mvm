/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_get_ammo.cpp
 * used in MvM: TODO
 * 
 * SuspendFor from CTFBotTacticalMonitor::Update
 * SuspendFor from CTFBotEngineerBuildSentryGun::Update
 * SuspendFor from CTFBotEngineerBuildDispenser::Update
 * SuspendFor from CTFBotEngineerBuildTeleportEntrance::Update
 * SuspendFor from CTFBotEngineerBuildTeleportExit::Update
 */


ConVar tf_bot_ammo_search_range("tf_bot_ammo_search_range", "5000", FCVAR_CHEAT,
	"How far bots will search to find ammo around them");
ConVar tf_bot_debug_ammo_scavanging("tf_bot_debug_ammo_scavanging", "0", FCVAR_CHEAT);


static CHandle<T> s_possibleAmmo; // TODO: T
static CTFBot *s_possibleBot;
static int s_possibleFrame;


CTFBotGetAmmo::CTFBotGetAmmo()
{
	this->m_PathFollower.Invalidate();
	// 4808 = nullptr
	// 480c = false
}

CTFBotGetAmmo::~CTFBotGetAmmo()
{
}


const char *CTFBotGetAmmo::GetName() const
{
	return "GetAmmo";
}


ActionResult<CTFBot> CTFBotGetAmmo::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	VPROF_BUDGET("CTFBotGetAmmo::OnStart", "NextBot");
	
	
	
	// TODO
}

ActionResult<CTFBot> CTFBotGetAmmo::Update(CTFBot *actor, float dt)
{
	if (actor->IsAmmoFull()) {
		return ActionResult<CTFBot>::Done("My ammo is full");
	}
	
	
	
	// TODO
}


EventDesiredResult<CTFBot> CTFBotGetAmmo::OnContact(CTFBot *actor, CBaseEntity *ent, CGameTrace *trace)
{
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotGetAmmo::OnMoveToSuccess(CTFBot *actor, const Path *path)
{
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotGetAmmo::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	return EventDesiredResult<CTFBot>::Done("Failed to reach ammo", ResultSeverity::CRITICAL);
}

EventDesiredResult<CTFBot> CTFBotGetAmmo::OnStuck(CTFBot *actor)
{
	return EventDesiredResult<CTFBot>::Done("Stuck trying to reach ammo", ResultSeverity::CRITICAL);
}


QueryResponse CTFBotGetAmmo::ShouldHurry(const INextBot *nextbot) const
{
	return QueryResponse::YES;
}


bool CTFBotGetAmmo::IsPossible(CTFBot *actor)
{
	VPROF_BUDGET("CTFBotGetAmmo::IsPossible", "NextBot");
	
	CUtlVector<CNavArea *> areas;
	CollectSurroundingAreas(&areas, actor->GetLastKnownArea(), tf_bot_ammo_search_range.GetFloat(),
		actor->GetLocomotionInterface()->GetStepHeight(),
		actor->GetLocomotionInterface()->GetDeathDropHeight());
	
	CUtlVector<CHandle<CBaseEntity>> *ammos = TFGameRules()->GetAmmoEntityVector();
	
	CBaseEntity *ammo = nullptr;
	float min_cost = FLT_MAX;
	FOR_EACH_VEC((*ammos), i) {
		CBaseEntity *ent = (*ammos)[i];
		if (ent == nullptr) continue;
		
		auto area = static_cast<CTFNavArea *>(TheNavMesh->GetNearestNavArea(ent->WorldSpaceCenter()));
		if (area == nullptr) continue;
		
		CClosestTFPlayer functor(ent->WorldSpaceCenter());
		ForEachPlayer(functor);
		if (functor.m_pPlayer != nullptr && !functor.m_pPlayer->InSameTeam(actor)) {
			continue;
		}
		
		// TODO: big giant if statement (func_regenerate .. obj_dispenser, etc)
		
		
	}
	
	// FClassnameIs
	
	// TODO
}


CAmmoFilter::CAmmoFilter(CBaseEntity *actor)
	: m_Actor(actor)
{
}


bool CAmmoFilter::IsSelected(const CBaseEntity *ent) const
{
	
	
	
	
	// TODO
}
