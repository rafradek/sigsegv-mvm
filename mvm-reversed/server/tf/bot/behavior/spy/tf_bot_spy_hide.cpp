/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/spy/tf_bot_spy_hide.cpp
 * used in MvM: TODO
 */


CTFBotSpyHide::CTFBotSpyHide(CTFPlayer *victim)
{
	this->m_hVictim = victim;
}

CTFBotSpyHide::~CTFBotSpyHide()
{
}


const char *CTFBotSpyHide::GetName() const
{
	return "SpyHide";
}


ActionResult<CTFBot> CTFBotSpyHide::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	/* BUG: doesn't set PathFollower's min lookahead distance */
	
	this->m_HidingSpot = nullptr;
	this->m_ctFindHidingSpot.Invalidate();
	this->m_bAtHidingSpot = false;
	
	/* assigns FLT_MAX instead if last known area is nullptr or if function returns negative */
	this->m_flEnemyIncursionDistance = actor->GetLastKnownArea()->GetIncursionDistance(GetEnemyTeam(actor));
	
	this->m_ctTeaseVictim.Start(RandomFloat(5.0f, 10.0f));
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSpyHide::Update(CTFBot *actor, float dt)
{
	if (this->m_hVictim != nullptr && !actor->GetVisionInterface()->IsIgnored(this->m_hVictim)) {
		return ActionResult<CTFBot>::SuspendFor(new CTFBotSpyAttack(this->m_hVictim),
			"Going after our initial victim");
	}
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
	if (threat != nullptr && threat->GetTimeSinceLastKnown() < 3.0f) {
		CTFPlayer *enemy = ToTFPlayer(threat->GetEntity());
		if (enemy != nullptr && actor->IsRangeLessThan(enemy, 750.0f) &&
			enemy->IsLookingTowards(actor, 0.9f)) {
			return ActionResult<CTFBot>::SuspendFor(new CTFBotSpyAttack(enemy),
				"Opportunistic attack or self defense!");
		}
	}
	
	if (this->m_ctTeaseVictim.IsElapsed()) {
		this->m_ctTeaseVictim.Start(RandomFloat(5.0f, 10.0f));
		
		actor->EmitSound("Spy.TeaseVictim");
	}
	
	if (this->m_bAtHidingSpot) {
		auto area = static_cast<CTFNavArea *>(actor->GetLastKnownArea());
		if (area != nullptr) {
			this->m_flEnemyIncursionDistance = area->GetIncursionDistance(GetEnemyTeam(actor));
		}
		
		return ActionResult<CTFBot>::SuspendFor(new CTFBotSpyLurk(),
			"Reached hiding spot - lurking");
	}
	
	if (this->m_HidingSpot == nullptr && this->m_ctFindHidingSpot.IsElapsed()) {
		this->FindHidingSpot(actor);
	}
	
	this->m_PathFollower.Update(actor);
	
	if (this->m_HidingSpot != nullptr && this->m_ctRecomputePath.IsElapsed()) {
		this->m_ctRecomputePath.Start(RandomFloat(0.3f, 0.5f));
		
		CTFBotPathCost cost_func(actor, SAFEST_ROUTE);
		this->m_PathFollower.Compute(actor, this->m_HidingSpot->GetPosition(), cost_func, 0.0f, true);
	}
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSpyHide::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_HidingSpot = nullptr;
	this->m_bAtHidingSpot = false;
	this->m_hVictim = nullptr;
	
	return ActionResult<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotSpyHide::OnMoveToSuccess(CTFBot *actor, const Path *path)
{
	this->m_bAtHidingSpot = true;
	
	return EventDesiredResult<CTFBot>::Continue(ResultSeverity::CRITICAL);
}

EventDesiredResult<CTFBot> CTFBotSpyHide::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	this->m_HidingSpot = nullptr;
	this->m_bAtHidingSpot = false;
	
	return EventDesiredResult<CTFBot>::Continue(ResultSeverity::MEDIUM);
}


QueryResponse CTFBotSpyHide::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	return QueryResponse::NO;
}


bool CTFBotSpyHide::FindHidingSpot(CTFBot *actor)
{
	if (actor->GetLastKnownArea() == nullptr) {
		return false;
	}
	
	this->m_HidingSpot = nullptr;
	
	CUtlVector<CNavArea *> nearby;
	CollectSurroundingAreas(&nearby, actor->GetLastKnownArea(), 3500.0f, 500.0f, 500.0f);
	
	CUtlSortVector<IncursionEntry_t, SpyHideIncursionDistanceLess> entries;
	
	/* this is almost certainly some mangled inlining stuff that we've done a
	 * relatively poor job of un-spaghettifying */
	
	float incursion_max;
	
	int enemy_team1 = actor->GetTeamNumber();
	int enemy_team2 = enemy_team1;
	
	if (enemy_team1 > 3) {
		incursion_max = 999999.0f;
	} else {
		if (enemy_team1 == TF_TEAM_RED) {
			enemy_team1 = TF_TEAM_BLUE;
			enemy_team2 = TF_TEAM_BLUE;
		} else if (enemy_team1 == TF_TEAM_BLUE) {
			enemy_team1 = TF_TEAM_RED;
			enemy_team2 = TF_TEAM_RED;
		}
		
		if (static_cast<CTFNavArea *>(actor->GetLastKnownArea())->GetIncursionDistance(enemy_team2) >= 0.0f) {
			incursion_max = this->m_flEnemyIncursionDistance + 1000.0f;
		}
	}
	
	if (enemy_team1 <= 3) {
		FOR_EACH_VEC(nearby, i) {
			auto area = static_cast<CTFNavArea *>(nearby[i]);
			
			if (!area->GetHidingSpots()->IsEmpty() &&
				area->GetIncursionDistance(enemy_team2) >= 0.0f && area->GetIncursionDistance(enemy_team2) <= incursion_max) {
				IncursionEntry_t entry;
			
				entry.teamnum = enemy_team2;
				entry.area    = area;
				
				entries.Insert(entry);
			}
		}
	}
	
	if (!entries.IsEmpty()) {
		this->m_HidingSpot = entries.Random().area->GetHidingSpots->Random();
		
		return true;
	}
	
	return false;
}


bool SpyHideIncursionDistanceLess::Less(const IncursionEntry_t& lhs, const IncursionEntry_t& rhs, void *ctx)
{
	/* not 100% sure if this is correct but it's at least close */
	return (lhs.area->GetIncursionDistance(lhs.teamnum) < rhs.area->GetIncursionDistance(rhs.teamnum));
}
