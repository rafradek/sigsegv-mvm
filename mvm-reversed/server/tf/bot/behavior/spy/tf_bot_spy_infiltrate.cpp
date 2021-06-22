/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/spy/tf_bot_spy_infiltrate.cpp
 * used in MvM: TODO
 */


ConVar tf_bot_debug_spy("tf_bot_debug_spy", "0", FCVAR_CHEAT);


CTFBotSpyInfiltrate::CTFBotSpyInfiltrate()
{
}

CTFBotSpyInfiltrate::~CTFBotSpyInfiltrate()
{
}


const char *CTFBotSpyInfiltrate::GetName() const
{
	return "SpyInfiltrate";
}


ActionResult<CTFBot> CTFBotSpyInfiltrate::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	/* BUG: doesn't set PathFollower's min lookahead distance */
	
	this->m_HidingArea = nullptr;
	this->m_bCloaked = false;
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSpyInfiltrate::Update(CTFBot *actor, float dt)
{
	CBaseCombatWeapon *revolver = actor->Weapon_GetSlot(0);
	if (revolver != nullptr) {
		actor->Weapon_Switch(revolver);
	}
	
	auto area = static_cast<CTFNavArea *>(actor->GetLastKnownArea());
	if (area == nullptr) {
		return ActionResult<CTFBot>::Continue();
	}
	
	if (!actor->m_Shared.IsStealthed() && !area->IsSpawnRoom() && area->IsInCombat() && !this->m_bCloaked) {
		this->m_bCloaked = true;
		actor->PressAltFireButton();
	}
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
	if (threat != nullptr && threat->GetEntity() != nullptr && threat->GetEntity()->IsBaseObject()) {
		auto obj = static_cast<CBaseObject *>(threat->GetEntity());
		if (!obj->HasSapper() && actor->IsEnemy(obj)) {
			return ActionResult<CTFBot>::SuspendFor(new CTFBotSpySap(obj),
				"Sapping an enemy object");
		}
	}
	
	if (actor->m_hTargetSentry != nullptr && !actor->m_hTargetSentry->HasSapper()) {
		return ActionResult<CTFBot>::SuspendFor(new CTFBotSpySap(actor->m_hTargetSentry),
			"Sapping a Sentry");
	}
	
	if (this->m_HidingArea == nullptr && this->m_ctFindHidingArea.IsElapsed()) {
		this->FindHidingSpot(actor);
		
		this->m_ctFindHidingArea.Start(3.0f);
	}
	
	if (!TFGameRules()->InSetup() && threat != nullptr && threat->GetTimeSinceLastKnown() < 3.0f) {
		CTFPlayer *victim = ToTFPlayer(threat->GetEntity());
		if (victim != nullptr) {
			auto victim_area = static_cast<CTFNavArea *>(victim->GetLastKnownArea());
			if (victim_area != nullptr && victim->GetTeamNumber() < TF_TEAM_COUNT &&
				victim_area->GetIncursionDistance(victim->GetTeamNumber()) > area->GetIncursionDistance(victim->GetTeamNumber())) {
				if (actor->m_Shared.IsStealthed()) {
					return ActionResult<CTFBot>::SuspendFor(new CTFBotRetreatToCover(new CTFBotSpyAttack(victim)),
						"Hiding to decloak before going after a backstab victim");
				} else {
					return ActionResult<CTFBot>::SuspendFor(new CTFBotSpyAttack(victim),
						"Going after a backstab victim");
				}
			}
		}
	}
	
	if (this->m_HidingArea == nullptr) {
		return ActionResult<CTFBot>::Continue();
	}
	
	if (tf_bot_debug_spy.GetBool()) {
		this->m_HidingArea->DrawFilled(0xff, 0xff, 0x00, 0xff, 0.0f, true, 5.0f);
	}
	
	if (this->m_HidingArea == area) {
		if (TFGameRules()->InSetup()) {
			this->m_ctWait.Start(RandomFloat(0.0f, 5.0f));
		} else {
			if (this->m_ctWait.HasStarted() && this->m_ctWait.IsElapsed()) {
				this->m_ctWait.Invalidate();
			} else {
				this->m_ctWait.Start(RandomFloat(5.0f, 10.0f));
			}
		}
	} else {
		if (this->m_ctRecomputePath.IsElapsed()) {
			this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
			
			CTFBotPathCost cost_func(actor, SAFEST_ROUTE);
			this->m_PathFollower.Compute(actor, this->m_HidingArea->GetCenter(), cost_func, 0.0f, true);
		}
		
		this->m_PathFollower.Update(actor);
		
		this->m_ctWait.Invalidate();
	}
	
	return ActionResult<CTFBot>::Continue();
}

void CTFBotSpyInfiltrate::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
}

ActionResult<CTFBot> CTFBotSpyInfiltrate::OnSuspend(CTFBot *actor, Action<CTFBot> *action)
{
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSpyInfiltrate::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_ctRecomputePath.Invalidate();
	
	return ActionResult<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotSpyInfiltrate::OnStuck(CTFBot *actor)
{
	this->m_HidingArea = nullptr;
	this->m_ctFindHidingArea.Invalidate();
	
	return ActionResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotSpyInfiltrate::OnTerritoryCaptured(CTFBot *actor, int i1)
{
	this->m_ctFindHidingArea.Start(5.0f);
	
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotSpyInfiltrate::OnTerritoryLost(CTFBot *actor, int i1)
{
	this->m_ctFindHidingArea.Start(5.0f);
	
	return EventDesiredResult<CTFBot>::Continue();
}


QueryResponse CTFBotSpyInfiltrate::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	return QueryResponse::NO;
}


bool CTFBotSpyInfiltrate::FindHidingSpot(CTFBot *actor)
{
	this->m_HidingArea = nullptr;
	
	if (actor->GetAliveDuration() < 5.0f && TFGameRules()->InSetup()) {
		return false;
	}
	
	CUtlVector<CTFNavArea *> *exits;
	if (actor->GetTeamNumber() == TF_TEAM_RED) {
		exits = &TheNavMesh->m_SpawnExits[TF_TEAM_BLUE];
	} else if (actor->GetTeamNumber() == TF_TEAM_BLUE) {
		exits = &TheNavMesh->m_SpawnExits[TF_TEAM_RED];
	}
	
	if (exits == nullptr || exits->IsEmpty()) {
		if (tf_bot_debug_spy.GetBool()) {
			DevMsg("%3.2f: No enemy spawn room exit areas found\n", gpGlobals->curtime);
		}
		
		return false;
	}
	
	CUtlVector<CTFNavArea *> surrounding;
	FOR_EACH_VEC(*exits, i) {
		CUtlVector<CTFNavArea *> temp;
		CollectSurroundingAreas(&temp, (*exits)[i], 2500.0f,
			actor->GetLocomotionInterface()->GetStepHeight(),
			actor->GetLocomotionInterface()->GetStepHeight());
		
		surrounding.AddVectorToTail(temp);
	}
	
	CUtlVector<CTFNavArea *> areas;
	FOR_EACH_VEC(surrounding, i) {
		if (!actor->GetLocomotionInterface()->IsAreaTraversable(surrounding[i])) {
			continue;
		}
		
		bool visible = false;
		FOR_EACH_VEC(exits, j) {
			if (surrounding[i]->IsPotentiallyVisible(exits[j])) {
				visible = true;
				break;
			}
		}
		
		if (!visible) {
			areas.AddToTail(surrounding[i]);
		}
	}
	
	if (areas.IsEmpty()) {
		if (tf_bot_debug_spy.GetBool()) {
			DevMsg("%3.2f: Can't find any non-visible hiding areas, "
				"trying for anything near the spawn exit...\n", gpGlobals->curtime);
		}
		
		FOR_EACH_VEC(surrounding, i) {
			if (actor->GetLocomotionInterface()->IsAreaTraversable(surrounding[i])) {
				areas.AddToTail(surrounding[i]);
			}
		}
		
		if (areas.IsEmpty()) {
			if (tf_bot_debug_spy.GetBool()) {
				DevMsg("%3.2f: Can't find any areas near the enemy spawn exit - "
					"just heading to the enemy spawn and hoping...\n", gpGlobals->curtime);
			}
			
			this->m_HidingArea = exits.Random();
			
			return false;
		}
	}
	
	this->m_HidingArea = areas.Random();
	
	return true;
}
