/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/spy/tf_bot_spy_sap.cpp
 * used in MvM: TODO
 */


CTFBotSpySap::CTFBotSpySap(CBaseObject *target)
{
	this->m_hTarget = target;
}

CTFBotSpySap::~CTFBotSpySap()
{
}


const char *CTFBotSpySap::GetName() const
{
	return "SpySap";
}


ActionResult<CTFBot> CTFBotSpySap::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	/* BUG: doesn't set PathFollower's min lookahead distance */
	
	actor->m_bLookAroundForEnemies = false;
	
	if (actor->m_Shared.IsStealthed()) {
		actor->PressAltFireButton();
	}
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSpySap::Update(CTFBot *actor, float dt)
{
	CBaseObject *new_target = actor->GetNearestKnownSappableTarget();
	if (new_target != nullptr) {
		this->m_hTarget = new_target;
	}
	
	if (this->m_hTarget == nullptr) {
		return ActionResult<CTFBot>::Done("Sap target gone");
	}
	
	CUtlVector<CKnownEntity> knowns;
	actor->GetVisionInterface()->CollectKnownEntities(&knowns);
	
	CTFPlayer *engie = nullptr;
	
	FOR_EACH_VEC(knowns, i) {
		CTFPlayer *player = ToTFPlayer(knowns[i]);
		if (player == nullptr) continue;
		
		/* why does this only consider the first known player? */
		if (actor->IsEnemy(player)) {
			engie = player;
			
			if (player->IsPlayerClass(TF_CLASS_ENGINEER) && this->m_hTarget->GetOwner() == player &&
				actor->IsRangeLessThan(player, 150.0f) && actor->IsEntityBetweenTargetAndSelf(engie, this->m_hTarget)) {
				return ActionResult<CTFBot>::SuspendFor(new CTFBotSpyAttack(engie),
					"Backstabbing the engineer before I sap his buildings");
			}
			
			break;
		}
	}
	
	if (actor->IsRangeLessThan(this->m_hTarget, 80.0f)) {
		/* TODO: enum? */
		CBaseCombatWeapon *sapper = actor->Weapon_GetWeaponByType(4);
		if (sapper == nullptr) {
			return ActionResult<CTFBot>::Done("I have no sapper");
		}
		actor->Weapon_Switch(sapper);
		
		if (actor->m_Shared.IsStealthed()) {
			actor->PressAltFireButton();
		}
		
		actor->GetBodyInterface()->AimHeadTowards(this->m_hTarget,
			IBody::LookAtPriorityType::OVERRIDE_ALL, 0.1f, nullptr, "Aiming my sapper");
		
		actor->PressFireButton();
	}
	
	if (!actor->IsRangeGreaterThan(this->m_hTarget, 40.0f)) {
		if (this->m_hTarget->HasSapper()) {
			CBaseObject *new_target = actor->GetNearestKnownSappableTarget();
			if (new_target != nullptr) {
				this->m_hTarget = new_target;
			} else {
				if (engie != nullptr && engie->IsPlayerClass(TF_CLASS_ENGINEER)) {
					return ActionResult<CTFBot>::SuspendFor(new CTFBotSpyAttack(engie),
						"Attacking an engineer");
				} else {
					return ActionResult<CTFBot>::Done("All targets sapped");
				}
			}
		}
		
		return ActionResult<CTFBot>::Continue();
	}
	
	if (this->m_ctRecomputePath.IsElapsed()) {
		this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
		
		CTFBotPathCost cost_func(actor, FASTEST_ROUTE);
		this->m_PathFollower.Compute(actor, this->m_hTarget, cost_func, 0.0f, true);
	}
	
	this->m_PathFollower.Update(actor);
	
	return ActionResult<CTFBot>::Continue();
}

void CTFBotSpySap::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
	actor->m_bLookAroundForEnemies = true;
}

ActionResult<CTFBot> CTFBotSpySap::OnSuspend(CTFBot *actor, Action<CTFBot> *action)
{
	actor->m_bLookAroundForEnemies = true;
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSpySap::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	actor->m_bLookAroundForEnemies = false;
	
	return ActionResult<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotSpySap::OnStuck(CTFBot *actor)
{
	return EventDesiredResult<CTFBot>::Done("I'm stuck, probably on a sapped building that hasn't exploded yet",
		ResultSeverity::CRITICAL);
}


QueryResponse CTFBotSpySap::ShouldRetreat(const INextBot *nextbot) const
{
	return QueryResponse::NO;
}

QueryResponse CTFBotSpySap::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	CTFBot *actor = ToTFBot(nextbot->GetEntity());
	
	if (this->m_hTarget == nullptr || this->m_hTarget->HasSapper()) {
		if (actor->m_Shared.InCond(TF_COND_DISGUISED) || actor->m_Shared.InCond(TF_COND_DISGUISING) || actor->m_Shared.IsStealthed()) {
			return this->AreAllDangerousSentriesSapped(actor);
		} else {
			return QueryResponse::YES;
		}
	} else {
		return QueryResponse::NO;
	}
}

QueryResponse CTFBotSpySap::IsHindrance(const INextBot *nextbot, CBaseEntity *it) const
{
	if (this->m_hTarget != nullptr && nextbot->IsRangeLessThan(this->m_hTarget, 300.0f)) {
		return QueryResponse::NO;
	}
	
	return QueryResponse::DONTCARE;
}


bool CTFBotSpySap::AreAllDangerousSentriesSapped(CTFBot *actor) const
{
	CUtlVector<CKnownEntity> knowns;
	actor->GetVisionInterface()->CollectKnownEntities(&knowns);
	
	FOR_EACH_VEC(knowns, i) {
		auto obj = dynamic_cast<CBaseObject *>(knowns[i]->GetEntity());
		if (obj == nullptr) continue;
		
		if (obj->ObjectType() == OBJ_SENTRYGUN && !obj->HasSapper() && actor->IsEnemy(obj) &&
			actor->IsRangeLessThan(obj, 1100.0f) && actor->IsLineOfFireClear(obj)) {
			return false;
		}
	}
	
	return true;
}
