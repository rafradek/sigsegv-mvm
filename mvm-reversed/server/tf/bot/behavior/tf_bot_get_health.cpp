/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_get_health.cpp
 * used in MvM: TODO
 * 
 * SuspendFor from CTFBotTacticalMonitor::Update
 */


ConVar tf_bot_health_critical_ratio("tf_bot_health_critical_ratio", "0.3", FCVAR_CHEAT);
ConVar tf_bot_health_ok_ratio("tf_bot_health_ok_ratio", "0.8", FCVAR_CHEAT);
ConVar tf_bot_health_search_near_range("tf_bot_health_search_near_range", "1000", FCVAR_CHEAT);
ConVar tf_bot_health_search_far_range("tf_bot_health_search_far_range", "2000", FCVAR_CHEAT);


static CHandle<T> s_possibleHealth; // TODO: T
static CTFBot *s_possibleBot;
static int s_possibleFrame;


CTFBotGetHealth::CTFBotGetHealth()
{
}

CTFBotGetHealth::~CTFBotGetHealth()
{
}


const char *CTFBotGetHealth::GetName() const
{
	return "GetHealth";
}


ActionResult<CTFBot> CTFBotGetHealth::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	VPROF_BUDGET("CTFBotGetHealth::OnStart", "NextBot");
	
	// TODO
}

ActionResult<CTFBot> CTFBotGetHealth::Update(CTFBot *actor, float dt)
{
	// TODO
}


EventDesiredResult<CTFBot> CTFBotGetHealth::OnMoveToSuccess(CTFBot *actor, const Path *path)
{
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotGetHealth::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	return EventDesiredResult<CTFBot>::Done("Failed to reach health kit", ResultSeverity::CRITICAL);
}

EventDesiredResult<CTFBot> CTFBotGetHealth::OnStuck(CTFBot *actor)
{
	return EventDesiredResult<CTFBot>::Done("Stuck trying to reach health kit", ResultSeverity::CRITICAL);
}


QueryResponse CTFBotGetHealth::ShouldHurry(const INextBot *nextbot) const
{
	return QueryResponse::YES;
}


bool CTFBotGetHealth::IsPossible(CTFBot *actor)
{
	VPROF_BUDGET("CTFBotGetHealth::IsPossible", "NextBot");
	
	if (actor->m_Shared.m_nNumHealers > 0 || TFGameRules()->IsMannVsMachineMode()) {
		return false;
	}
	
	float ratio = Clamp((((float)this->GetHealth() / (float)this->GetMaxHealth()) - tf_bot_health_critical_ratio.GetFloat()) /
		(tf_bot_health_ok_ratio.GetFloat() - tf_bot_health_critical_ratio.GetFloat()), 0.0f, 1.0f);
	
	// TODO
}


CHealthFilter::CHealthFilter(CBaseEntity *actor)
	: m_Actor(actor)
{
}


bool CHealthFilter::IsSelected(const CBaseEntity *ent) const
{
	if (ent == nullptr) {
		return false;
	}
	
	CNavArea *nearest = TheNavMesh->GetNearestNavArea(ent->WorldSpaceCenter());
	if (nearest == nullptr) {
		return false;
	}
	
	// TODO
	
	// CClosestTFPlayer is a class
	// size >= 0x10
	// has operator()
}
