/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/spy/tf_bot_spy_leave_spawn_room.cpp
 * used in MvM: TODO
 */


CTFBotSpyLeaveSpawnRoom::CTFBotSpyLeaveSpawnRoom()
{
}

CTFBotSpyLeaveSpawnRoom::~CTFBotSpyLeaveSpawnRoom()
{
}


const char *CTFBotSpyLeaveSpawnRoom::GetName() const
{
	return "SpyLeaveSpawnRoom";
}


ActionResult<CTFBot> CTFBotSpyLeaveSpawnRoom::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	/* BUG: doesn't set PathFollower's min lookahead distance */
	
	actor->DisguiseAsMemberOfEnemyTeam();
	actor->PressAltFireButton();
	
	this->m_ctTeleport.Start(2.0f + RandomFloat(0.0f, 1.0f));
	this->m_nDistance = 0;
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSpyLeaveSpawnRoom::Update(CTFBot *actor, float dt)
{
	VPROF_BUDGET("CTFBotSpyLeaveSpawnRoom::Update", "NextBot");
	
	if (!this->m_ctTeleport.IsElapsed()) {
		return ActionResult<CTFBot>::Continue();
	}
	
	CUtlVector<CTFPlayer *> enemies;
	CollectPlayers<CTFPlayer>(&enemies, GetEnemyTeam(actor), true, false);
	
	/* BUG: what exactly is the point of doing a copy here? */
	CUtlVector<CTFPlayer *> enemies2 = enemies;
	if (enemies2.Count() > 1) {
		enemies2.Shuffle();
	}
	
	FOR_EACH_VEC(enemies2, i) {
		CTFPlayer *enemy = enemies2[i];
		if (TeleportNearVictim(actor, enemy, this->m_nDistance)) {
			return ChangeTo(new CTFBotSpyHide(enemy), "Hiding!");
		}
	}
	
	/* retry later */
	++this->m_nDistance;
	this->m_ctTeleport.Start(1.0f);
	return ActionResult<CTFBot>::Continue();
}


QueryResponse CTFBotSpyLeaveSpawnRoom::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	return QueryResponse::NO;
}


bool TeleportNearVictim(CTFBot *spy, CTFPlayer *victim, int dist)
{
	VPROF_BUDGET("CTFBotSpyLeaveSpawnRoom::TeleportNearVictim", "NextBot");
	
	if (victim == nullptr || victim->GetLastKnownArea() == nullptr) {
		return false;
	}
	
	float dist_limit = Min((500.0f * dist) + 1500.0f, 6000.0f);
	
	CUtlVector<CTFNavArea *> good_areas;
	
	CUtlVector<CNavArea *> near_areas;
	CollectSurroundingAreas(&near_areas, victim->GetLastKnownArea(), dist_limit,
		StepHeight, StepHeight);
	
	FOR_EACH_VEC(near_areas, i) {
		CTFNavArea *area = static_cast<CTFNavArea *>(near_areas[i]);
		
		if (area->IsValidForWanderingPopulation() &&
			!area->IsPotentiallyVisibleToTeam(victim->GetTeamNumber())) {
			good_areas.AddToTail(area);
		}
	}
	
	int limit = Max(good_areas.Count(), 10);
	for (int i = 0; i < limit; ++i) {
		CTFNavArea *area = good_areas.Random();
		
		Vector pos = {
			.x = area->GetCenter().x,
			.y = area->GetCenter().y,
			.z = area->GetCenter().z + StepHeight,
		};
		
		if (IsSpaceToSpawnHere(pos)) {
			spy->Teleport(pos, vec3_angle, vec3_origin);
			return true;
		}
	}
	
	return false;
}
