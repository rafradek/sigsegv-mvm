/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/engineer/mvm_engineer/tf_bot_mvm_engineer_idle.cpp
 * used in MvM: TODO
 */


ConVar tf_bot_engineer_mvm_sentry_hint_bomb_forward_range("tf_bot_engineer_mvm_sentry_hint_bomb_forward_range", "0", FCVAR_CHEAT);
ConVar tf_bot_engineer_mvm_sentry_hint_bomb_backward_range("tf_bot_engineer_mvm_sentry_hint_bomb_backward_range", "3000", FCVAR_CHEAT);
ConVar tf_bot_engineer_mvm_hint_min_distance_from_bomb("tf_bot_engineer_mvm_hint_min_distance_from_bomb", "1300", FCVAR_CHEAT);

CON_COMMAND_F(tf_bot_mvm_show_engineer_hint_region, "Show the nav areas MvM engineer bots will consider when selecting sentry and teleporter hints", FCVAR_CHEAT)
{
	// TODO
}


CTFBotMvMEngineerIdle::CTFBotMvMEngineerIdle()
{
}

CTFBotMvMEngineerIdle::~CTFBotMvMEngineerIdle()
{
}


const char *CTFBotMvMEngineerIdle::GetName() const
{
	return "MvMEngineerIdle";
}


ActionResult<CTFBot> CTFBotMvMEngineerIdle::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
	
	actor->m_bLookAroundForEnemies = false;
	
	this->m_hHintSentry = nullptr;
	this->m_hHintTele   = nullptr;
	this->m_hHintNest   = nullptr;
	
	this->m_nTeleportAttempts = 0;
	
	this->m_bTeleportedToHint         = false;
	this->m_bTriedToDetonateStaleNest = false;
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotMvMEngineerIdle::Update(CTFBot *actor, float dt)
{
	if (!actor->IsAlive()) {
		return ActionResult<CTFBot>::Done();
	}
	
	// TODO: slot enum
	CBaseCombatWeapon *w_melee = bot->Weapon_GetSlot(2);
	if (w_melee != nullptr) {
		actor->Weapon_Switch(w_melee);
	}
	
	if (this->m_hHintNest == nullptr || this->ShouldAdvanceNestSpot(actor)) {
		if (this->m_ctFindNestHint.HasStarted() && !this->m_ctFindNestHint.IsElapsed()) {
			return ActionResult<CTFBot>::Continue();
		}
		this->m_ctFindNestHint.Start(RandomFloat(1.0f, 2.0f));
		
		bool box_check       = false;
		bool out_of_range_ok = true;
		if ((actor->m_nBotAttrs & CTFBot::AttributeType::TELEPORTTOHINT) != 0) {
			if (!this->m_bTeleportedToHint) {
				box_check = true;
			}
			out_of_range_ok = false;
		}
		
		CHandle<CTFBotHintEngineerNest> h_nest;
		if (!CTFBotMvMEngineerHintFinder::FindHint(box_check, out_of_range_ok, &h_nest)) {
			return ActionResult<CTFBot>::Continue();
		}
		
		if (this->m_hHintNest != nullptr) {
			this->m_hHintNest->SetOwnerEntity(nullptr);
		}
		
		this->m_hHintNest = h_nest;
		h_nest->SetOwnerEntity(actor);
		
		this->m_hHintSentry = this->m_hHintNest->GetSentryHint();
		this->TakeOverStaleNest(this->m_hHintSentry, actor);
		
		if (!actor->m_TeleportWhere.IsEmpty()) {
			this->m_hHintTele = this->m_hHintNest->GetTeleporterHint();
			this->TakeOverStaleNest(this->m_hHintTele, actor);
		}
	}
	
	if (!this->m_bTeleportedToHint &&
		(actor->m_nBotAttrs & CTFBot::AttributeType::TELEPORTTOHINT) != 0) {
		this->m_bTeleportedToHint = true;
		++this->m_nTeleportAttempts;
		
		return ActionResult<CTFBot>::SuspendFor(new CTFBotMvMEngineerTeleportSpawn(
			this->m_hHintNest, (this->m_nTeleportAttempts == 1),
			"In spawn area - teleport to the teleporter hint"));
	}
	
	CObjectSentrygun *sentry = nullptr;
	
	if (this->m_hHintSentry != nullptr) {
		if (this->m_hHintSentry->GetOwnerEntity() != nullptr &&
			this->m_hHintSentry->GetOwnerEntity()->IsBaseObject()) {
			/* sentry exists; start the retreat timer so we will retreat if we
			 * should lose it in the future */
			this->m_ctSentryCooldown.Start(3.0f);
			
			sentry = static_cast<CObjectSentrygun *>(this->m_hHintSentry->GetOwnerEntity());
		} else {
			if (this->m_hHintSentry->GetOwnerEntity() != nullptr &&
				this->m_hHintSentry->GetOwnerEntity()->IsBaseObject()) {
				/* not sure under what circumstances this code would actually be
				 * reached; perhaps the static_cast in the if statement earlier
				 * is actually an assert_cast and they were naively expecting it
				 * to return nullptr if the owner wasn't a sentry? */
				sentry = static_cast<CObjectSentrygun *>(this->m_hHintSentry->GetOwnerEntity());
				sentry->SetOwnerEntity(actor);
			} else {
				/* do not have a sentry; retreat for a few seconds if we had a
				 * sentry before this; then build a new sentry */
				
				if (this->m_ctSentryCooldown.IsElapsed()) {
					return ActionResult<CTFBot>::SuspendFor(
						new CTFBotMvMEngineerBuildSentryGun(this->m_hHintSentry),
						"No sentry - building a new one");
				} else {
					return ActionResult<CTFBot>::SuspendFor(
						new CTFBotRetreatToCover(1.0f),
						"Lost my sentry - retreat!");
				}
			}
		}
		
		/* NOTE: this is m_flHealth, not m_iHealth */
		if (sentry->GetMaxHealth() > sentry->m_flHealth && !sentry->m_bBuilding) {
			this->m_ctSentrySafe.Start(3.0f);
		}
	}
	
	CObjectTeleporter *tele = nullptr;
	
	if (this->m_hHintTele != nullptr && this->m_ctSentrySafe.IsElapsed()) {
		if (this->m_hHintTele->GetOwnerEntity() != nullptr &&
			this->m_hHintTele->GetOwnerEntity()->IsBaseObject()) {
			this->m_ctTeleCooldown.Start(3.0f);
			
			tele = static_cast<CObjectTeleporter *>(this->m_hHintTele->GetOwnerEntity());
		} else {
			if (this->m_ctTeleCooldown.IsElapased()) {
				return ActionResult<CTFBot>::SuspendFor(
					new CTFBotMvMEngineerBuildTeleportExit(this->m_hHintTele),
					"Sentry is safe - building a teleport exit");
			}
		}
	}
	
	if (tele != nullptr && this->m_ctSentrySafe.IsElapsed()) {
		/* NOTE: this is m_flHealth, not m_iHealth */
		if (tele->GetMaxHealth() > tele->m_flHealth && !tele->m_bBuilding) {
			float dist = actor->GetAbsOrigin().DistTo(tele->GetAbsOrigin());
			
			if (dist < 90.0f) {
				actor->PressCrouchButton();
			}
			
			if (this->m_ctRecomputePath.IsElapsed()) {
				this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
				
				Vector dir = (tele->GetAbsOrigin() - actor->GetAbsOrigin());
				dir.NormalizeInPlace();
				
				Vector goal = tele->GetAbsOrigin() - (50.0f * dir);
				
				CTFBotPathCost cost_func(actor, SAFEST_ROUTE);
				this->m_PathFollower.Compute(actor, goal, cost_func, 0.0f, true);
			}
			
			this->m_PathFollower.Update(actor);
			
			if (dist < 75.0f) {
				actor->GetBodyInterface()->AimHeadTowards(tele->WorldSpaceCenter(),
					IBody::LookAtPriorityType::CRITICAL, 1.0f, nullptr, "Work on my Teleporter");
				actor->PressFireButton();
			}
			
			this->TryToDetonateStaleNest();
			return ActionResult<CTFBot>::Continue();
		}
	}
	
	if (sentry != nullptr) {
		float dist = actor->GetAbsOrigin().DistTo(sentry->GetAbsOrigin());
		
		if (dist < 90.0f) {
			actor->PressCrouchButton();
		}
		
		if (this->m_ctRecomputePath.IsElapsed()) {
			this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
			
			Vector dir;
			AngleVectors(sentry->GetTurretAngles(), &dir);
			
			Vector goal = sentry->GetAbsOrigin() - (50.0f * dir);
			
			CTFBotPathCost cost_func(actor, SAFEST_ROUTE);
			this->m_PathFollower.Compute(actor, goal, cost_func, 0.0f, true);
		}
		
		this->m_PathFollower.Update(actor);
		
		if (dist < 75.0f) {
			actor->GetBodyInterface()->AimHeadTowards(sentry->WorldSpaceCenter(),
				IBody::LookAtPriorityType::CRITICAL, 1.0f, nullptr, "Work on my Sentry");
			actor->PressFireButton();
		}
		
		this->TryToDetonateStaleNest();
		return ActionResult<CTFBot>::Continue();
	}
	
	this->TryToDetonateStaleNest();
	return ActionResult<CTFBot>::Continue();
}


QueryResponse CTFBotMvMEngineerIdle::ShouldHurry(const INextBot *nextbot) const
{
	return QueryResponse::YES;
}

QueryResponse CTFBotMvMEngineerIdle::ShouldRetreat(const INextBot *nextbot) const
{
	return QueryResponse::NO;
}

QueryResponse CTFBotMvMEngineerIdle::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	return QueryResponse::NO;
}


bool CTFBotMvMEngineerIdle::ShouldAdvanceNestSpot(CTFBot *actor)
{
	if (this->m_hHintNest == nullptr) return false;
	
	if (!this->m_ctAdvanceNestSpot.HasStarted()) {
		this->m_ctAdvanceNestSpot.Start(5.0f);
		return false;
	}
	
	for (int i = 0; i < actor->GetObjectCount(); ++i) {
		CBaseObject *obj = actor->GetObject(i);
		if (obj == nullptr) continue;
		
		if (obj->m_flHealth < obj->GetMaxHealth()) {
			this->m_ctAdvanceNestSpot.Start(5.0f);
			return false;
		}
	}
	
	if (this->m_ctAdvanceNestSpot.IsElapsed()) {
		this->m_ctAdvanceNestSpot.Invalidate();
	}
	
	BonbInfo_t bombinfo;
	if (!GetBombInfo(&bombinfo)) return false;
	
	CTFNavArea *hint_area = TheNavMesh->GetNearestNavArea(this->m_hHintNest->GetAbsOrigin(), false, 1000.0f);
	if (hint_area == nullptr) return false;
	
	return (hint_area->m_flBombTargetDistance > bombinfo.hatch_dist_fwd);
}

void CTFBotMvMEngineerIdle::TakeOverStaleNest(CBaseTFBotHintEntity *hint, CTFBot *actor)
{
	if (hint == nullptr || hint->OwnerObjectHasNoOwner()) {
		return;
	}
	
	CBaseObject *obj = static_cast<CBaseObject *>(hint->GetOwnerEntity());
	obj->SetOwnerEntity(actor);
	obj->SetBuilder(actor);
	actor->AddObject(obj);
}

void CTFBotMvMEngineerIdle::TryToDetonateStaleNest()
{
	if (this->m_bTriedToDetonateStaleNest) {
		return;
	}
	
	CTFBotHintSentrygun *hint_sentry = this->m_hHintSentry;
	if (hint_sentry == nullptr || hint_sentry->OwnerObjectFinishBuilding()) {
		CTFBotHintTeleporterExit *hint_tele = this->m_hHintTele;
		if (hint_tele == nullptr || hint_tele->OwnerObjectFinishBuilding()) {
			CUtlVector<CTFBotHintEngineerNest *> nests;
			
			for (int i = 0; i < ITFBotHintEntityAutoList::AutoList().Count(); ++i) {
				CBaseTFBotHintEntity *hint = static_cast<CBaseTFBotHintEntity *>(
					ITFBotHintEntityAutoList::AutoList()[i]);
				if (hint->GetHintType() != CBaseTFBotHintEntity::HintType::ENGINEER_NEST || nest->m_bDisabled || nest->GetOwnerEntity() == nullptr) {
					continue;
				}
				
				nests.AddToTail(static_cast<CTFBotHintEngineerNest *>(hint));
			}
			
			FOR_EACH_VEC(nests, i) {
				if (nests[i]->IsStaleNest()) {
					nests[i]->DetonateStaleNest();
				}
			}
			
			this->m_bTriedToDetonateStaleNest = true;
		}
	}
}


bool CTFBotMvMEngineerHintFinder::FindHint(bool box_check, bool out_of_range_ok, CHandle<CTFBotHintEngineerNest> *the_hint)
{
	CUtlVector<CTFBotHintEngineerNest *> hints;
	
	for (int i = 0; i < ITFBotHintEntityAutoList::AutoList().Count(); ++i) {
		CBaseTFBotHintEntity *hint = static_cast<CBaseTFBotHintEntity *>(
			ITFBotHintEntityAutoList::AutoList()[i]);
		
		if (hint->GetHintType() == CBaseTFBotHintEntity::HintType::ENGINEER_NEST &&
			!hint->m_bDisabled && hint->GetOwnerEntity() == nullptr) {
			hints.AddToTail(static_cast<CTFBotHintEngineerNest *>(hint));
		}
	}
	
	BombInfo_t bomb_info;
	GetBombInfo(&bomb_info);
	
	CUtlVector<CTFBotHintEngineerNest *> hints1; // in fwd~back, stale
	CUtlVector<CTFBotHintEngineerNest *> hints2; // in fwd~back, within min bomb distance
	CUtlVector<CTFBotHintEngineerNest *> hints3; // in fwd~infinity
	CUtlVector<CTFBotHintEngineerNest *> hints4; // others
	
	FOR_EACH_VEC(hints, i) {
		CTFBotHintEngineerNest *hint = hints[i];
		
		CTFNavArea *area = TheNavMesh->GetNearestNavArea(hint->GetAbsOrigin());
		if (area == nullptr) {
			Warning("Sentry hint has NULL nav area!\n");
			continue;
		}
		
		float dist = area->m_flBombTargetDistance;
		if (dist > bomb_info.hatch_dist_fwd && dist < bomb_info.hatch_dist_back) {
			CBaseEntity *pList[256];
			if (box_check && UTIL_EntitiesInBox(pList, 256,
				hint->GetAbsOrigin() + VEC_HULL_MIN,
				hint->GetAbsOrigin() + VEC_HULL_MAX,
				FL_OBJECT | FL_FAKECLIENT) > 0) {
				continue;
			}
			
			if (hint->IsStaleNest()) {
				hints1.AddToTail(hint);
			} else {
				if (hint->GetAbsOrigin()->DistTo(bomb_info.closest_pos) >=
					tf_bot_engineer_mvm_hint_min_distance_from_bomb.GetFloat()) {
					hints2.AddToTail(hint);
				}
			}
		} else if (dist > bomb_info.hatch_dist_back) {
			hints3.AddToTail(hint);
		} else {
			hints4.AddToTail(hint);
		}
	}
	
	CTFBotHintEngineerNest *hint;
	if (!hints1.IsEmpty()) {
		hint = hints1.Random();
	} else if (!hints2.IsEmpty()) {
		hint = hints2.Random();
	} else if (out_of_range_ok) {
		hint = SelectOutOfRangeNest(hints3);
		if (hint == nullptr) {
			hint = SelectOutOfRangeNest(hints4);
		}
	}
	
	if (the_hint != nullptr) {
		*the_hint = hint;
	}
	
	return (hint != nullptr);
}


CTFBotHintEngineerNest *SelectOutOfRangeNest(const CUtlVector<CTFBotHintEngineerNest *>& nests)
{
	if (nests.IsEmpty()) {
		return nullptr;
	}
	
	FOR_EACH_VEC(nests, i) {
		CTFBotHintEngineerNest *nest = nests[i];
		
		if (nest->IsStaleNest()) {
			return nest;
		}
	}
	
	return nests.Random();
}

bool GetBombInfo(BombInfo_t *info)
{
	float hatch_dist = 0.0f;
	
	FOR_EACH_VEC(TheNavAreas, i) {
		CTFNavArea *area = static_cast<CTFNavArea *>(TheNavAreas[i]);
		
		if ((area->m_nAttributes & (BLUE_SPAWN_ROOM | RED_SPAWN_ROOM)) != 0) {
			continue;
		}
		
		hatch_dist = Max(Max(area->m_flBombTargetDistance, hatch_dist), 0.0f);
	}
	
	CCaptureFlag *closest_flag = nullptr;
	float closest_flag_x;
	float closest_flag_y;
	float closest_flag_z;
	
	for (int i = 0; i < ICaptureFlagAutoList::AutoList().Count(); ++i) {
		CCaptureFlag *flag = static_cast<CCaptureFlag *>(ICaptureFlagAutoList::AutoList()[i]);
		
		Vector flag_pos;
		
		CTFPlayer *owner = ToTFPlayer(flag->GetOwnerEntity());
		if (owner != nullptr) {
			flag_pos = owner->GetAbsOrigin();
		} else {
			flag_pos = flag->WorldSpaceCenter();
		}
		
		CTFNavArea *area = TheNavMesh->GetNearestNavArea(flag_pos);
		if (area != nullptr && area->m_flBombTargetDistance < hatch_dist) {
			closest_flag = flag;
			hatch_dist = area->m_flBombTargetDistance;
			closest_flag_pos = flag_pos;
		}
	}
	
	bool success = (closest_flag != nullptr);
	
	float range_back = tf_bot_engineer_mvm_sentry_hint_bomb_backward_range.GetFloat();
	float range_fwd  = tf_bot_engineer_mvm_sentry_hint_bomb_forward_range.GetFloat();
	
	if (info != nullptr) {
		info->closest_pos = {
			.x = closest_flag_x,
			.y = closest_flag_y,
			.z = closest_flag_z,
		};
		info->hatch_dist_back = hatch_dist + range_back;
		info->hatch_dist_fwd  = hatch_dist - range_fwd;
	}
	
	return success;
}
