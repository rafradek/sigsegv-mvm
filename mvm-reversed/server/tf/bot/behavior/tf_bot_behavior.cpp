/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_behavior.cpp
 * used in MvM: TODO
 * 
 * main action of CTFBotIntention
 */


ConVar tf_bot_path_lookahead_range("tf_bot_path_lookahead_range", "300", FCVAR_NONE);
ConVar tf_bot_sniper_aim_error("tf_bot_sniper_aim_error", "0.01", FCVAR_CHEAT);
ConVar tf_bot_sniper_aim_steady_rate("tf_bot_sniper_aim_steady_rate", "10", FCVAR_CHEAT);
ConVar tf_bot_debug_sniper("tf_bot_debug_sniper", "0", FCVAR_CHEAT);
ConVar tf_bot_fire_weapon_min_time("tf_bot_fire_weapon_min_time", "1", FCVAR_CHEAT);
ConVar tf_bot_taunt_victim_chance("tf_bot_taunt_victim_chance", "20", FCVAR_NONE);
ConVar tf_bot_notice_backstab_chance("tf_bot_notice_backstab_chance", "25", FCVAR_CHEAT);
ConVar tf_bot_notice_backstab_min_chance("tf_bot_notice_backstab_min_chance", "100", FCVAR_CHEAT);
ConVar tf_bot_notice_backstab_max_chance("tf_bot_notice_backstab_max_chance", "750", FCVAR_CHEAT);
ConVar tf_bot_arrow_elevation_rate("tf_bot_arrow_elevation_rate", "0.0001", FCVAR_CHEAT,
	"When firing arrows at far away targets, this is the degree/range slope to raise our aim");
ConVar tf_bot_ballistic_elevation_rate("tf_bot_ballistic_elevation_rate", "0.01", FCVAR_CHEAT,
	"When lobbing grenades at far away targets, this is the degree/range slope to raise our aim");
ConVar tf_bot_hitscan_range_limit("tf_bot_hitscan_range_limit", "1800", FCVAR_CHEAT);
ConVar tf_bot_always_full_reload("tf_bot_always_full_reload", "0", FCVAR_CHEAT);
ConVar tf_bot_fire_weapon_allowed("tf_bot_fire_weapon_allowed", "1", FCVAR_CHEAT,
	"If zero, TFBots will not pull the trigger of their weapons (but will act like they did)");


CTFBotMainAction::CTFBotMainAction(/* TODO */)
{
	// TODO
}
 
CTFBotMainAction::~CTFBotMainAction()
{
}


const char *CTFBotMainAction::GetName() const
{
	return "MainAction";
}


ActionResult<CTFBot> CTFBotMainAction::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	// TODO: handle @ 0x68 = nullptr
	
	// TODO
	
	this->m_bReloadingBarrage = false;
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotMainAction::Update(CTFBot *actor, float dt)
{
	VPROF_BUDGET("CTFBotMainAction::Update", "NextBot");
	
	if (!actor->IsAlive()) {
		return ActionResult<CTFBot>::ChangeTo(new CTFBotDead(), "I died!");
	}
	
	if (actor->GetTeamNumber() != TF_TEAM_BLUE &&
		actor->GetTeamNumber() != TF_TEAM_RED) {
		return ActionResult<CTFBot>::Done("Not on a playing team");
	}
	
	if (actor->FindPartnerTauntInitiator()) {
		return ActionResult<CTFBot>::SuspendFor(new CTFBotTaunt(), "Responding to teammate partner taunt");
	}
	
	actor->GetVisionInterface()->SetFieldOfView(actor->GetFOV());
	
	if (TFGameRules()->IsInTraining() && actor->GetTeamNumber() == TF_TEAM_BLUE) {
		// TODO: enum for GiveAmmo param 2?
		// TODO: default val for GiveAmmo param 3?
		actor->GiveAmmo(1000, 3, true);
	}
	
	// TODO: tf_bot_sniper_aim_steady_rate EyeAngles IntervalTimer stuff
	
	if (TFGameRules()->IsMannVsMachineMode() && actor->GetTeamNumber() == TF_TEAM_BLUE) {
		// TODO: enum for param 2, default for param 3
		actor->GiveAmmo(100, 3, true);
		actor->m_Shared.AddToSpyCloakMeter(100.0, false);
		
		CTFNavArea *area = actor->GetLastKnownArea();
		if (area != nullptr) {
			if ((area->m_nAttributes & (actor->GetTeamNumber() == TF_TEAM_RED ?
				RED_SPAWN_ROOM : BLUE_SPAWN_ROOM)) != 0) {
				actor->m_Shared.AddCond(TF_COND_INVULNERABLE,                     0.5f);
				actor->m_Shared.AddCond(TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED, 0.5f);
				actor->m_Shared.AddCond(TF_COND_INVULNERABLE_WEARINGOFF,          0.5f);
			}
			
			if (area->GetZ(actor->GetAbsOrigin()) - actor->GetAbsOrigin().z > 100.0f) {
				if (!this->m_itUnderground.HasStarted()) {
					this->m_itUnderground.Start();
				} else {
					if (this->m_itUnderground.IsGreaterThen(3.0f)) {
						UTIL_LogPrintf("\"%s<%i><%s><%s>\" underground (position \"%3.2f %3.2f %3.2f\")\n",
							/* TODO */);
						actor->SetAbsOrigin(area->GetCenter());
					}
				}
			}
		} else {
			this->m_itUnderground.Invalidate();
		}
		
		if (actor->ShouldAutoJump()) {
			actor->GetLocomotionInterface()->Jump();
		}
	}
	
	if (!actor->IsFiringWeapon()) {
		// TODO
	}
	
	actor->EquipRequiredWeapon();
	actor->UpdateLookingAroundForEnemies();
	this->FireWeaponAtEnemy(actor);
	this->Dodge(actor);
	
	// actor + 0x271c + 0x9b = !actor->IsPlayerClass(TF_CLASS_DEMOMAN)
	
	return ActionResult<CTFBot>::Continue();
}


Action<CTFBot> *CTFBotMainAction::InitialContainedAction(CTFBot *actor)
{
	// TODO: get ctor args for CTFBotTacticalMonitor
	return new CTFBotTacticalMonitor(/* TODO */);
}


EventDesiredResult<CTFBot> CTFBotMainAction::OnContact(CTFBot *actor, CBaseEntity *ent, CGameTrace *trace)
{
	if (ent != nullptr &&
		(ent->CollisionProp()->GetSolidFlags() & FSOLID_NOT_SOLID) == 0 &&
		ENTINDEX(ent) != 0 &&
		!ent->IsPlayer()) {
		// TODO: set handle @ +0x68 = ent
		// TODO: set float  @ +0x6c = gpGlobals->curtime
		
		if (TFGameRules()->IsMannVsMachineMode() && actor->IsMiniBoss() &&
			ent->IsBaseObject()) {
			CBaseObject *obj = static_cast<CBaseObject *>(ent);
			if (obj->GetType() != OBJ_SENTRYGUN || obj->m_bMiniBuilding) {
				/* it looks suspiciously like they copy-and-pasted this from the
				 * sentry buster detonation damage code */
				
				float health = obj->GetMaxHealth();
				if (obj->GetHealth() < obj->GetMaxHealth()) {
					health = obj->GetMaxHealth();
				}
				
				CTakeDamage dmginfo(actor, actor, 4 * health, DMG_BLAST);
				CalculateMeleeDamageForce(&dmginfo, obj->WorldSpaceCenter() -
					actor->WorldSpaceCenter(), actor->WorldSpaceCenter());
				obj->TakeDamage(dmginfo);
			}
		}
	}
	
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotMainAction::OnStuck(CTFBot *actor)
{
	// TODO
}

EventDesiredResult<CTFBot> CTFBotMainAction::OnInjured(CTFBot *actor, const CTakeDamageInfo& info)
{
	// TODO
}

EventDesiredResult<CTFBot> CTFBotMainAction::OnOtherKilled(CTFBot *actor, CBaseCombatCharacter *who, const CTakeDamageInfo& info)
{
	// TODO
}


QueryResponse CTFBotMainAction::ShouldHurry(const INextBot *nextbot) const
{
	if (g_pPopulationManager == nullptr) {
		return QueryResponse::DONTCARE;
	}
	
	CTFBot *actor = ToTFBot(nextbot->GetEntity());
	
	CTFNavArea *area = static_cast<CTFNavArea *>(actor->GetLastKnownArea());
	if (area == nullptr) {
		return QueryResponse::DONTCARE;
	}
	
	if ((area->m_nAttributes & (actor->GetTeamNumber() == TF_TEAM_RED ?
		RED_SPAWN_ROOM : BLUE_SPAWN_ROOM)) != 0) {
		if (!g_pPopulationManager->m_bCanBotsAttackWhileInSpawnRoom) {
			return QueryResponse::YES;
		}
	}
	
	return QueryResponse::DONTCARE;
}

QueryResponse CTFBotMainAction::ShouldRetreat(const INextBot *nextbot) const
{
	CTFBot *actor = static_cast<CTFBot *>(nextbot->GetEntity());
	
	if (TheTFBots().IsMeleeOnly() || actor->m_Shared.IsInvulnerable() ||
		(actor->m_nBotAttrs & CTFBot::AttributeType::IGNOREENEMIES) != 0) {
		return QueryResponse::NO;
	}
	
	if (actor->m_Shared.IsControlStunned() || actor->m_Shared.IsLoserStateStunned()) {
		return QueryResponse::YES;
	}
	
	if (TFGameRules()->InSetup()) {
		return QueryResponse::NO;
	}
	
	if (actor->IsPlayerClass(TF_CLASS_SPY) &&
		(actor->m_Shared.InCond(TF_COND_DISGUISED) ||
		actor->m_Shared.InCond(TF_COND_DISGUISING) ||
		actor->m_Shared.IsStealthed())) {
		return QueryResponse::NO;
	}
	
	// TODO
}

QueryResponse CTFBotMainAction::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	if (g_pPopulationManager == nullptr) {
		return QueryResponse::YES;
	}
	
	CTFBot *actor = ToTFBot(nextbot->GetEntity());
	
	CTFNavArea *area = static_cast<CTFNavArea *>(actor->GetLastKnownArea());
	if (area == nullptr) {
		return QueryResponse::YES;
	}
	
	if ((area->m_nAttributes & (actor->GetTeamNumber() == TF_TEAM_RED ?
		RED_SPAWN_ROOM : BLUE_SPAWN_ROOM)) != 0) {
		if (!g_pPopulationManager->m_bCanBotsAttackWhileInSpawnRoom) {
			return QueryResponse::NO;
		}
	}
	
	return QueryResponse::YES;
}

Vector CTFBotMainAction::SelectTargetPoint(const INextBot *nextbot, const CBaseCombatCharacter *them) const
{
	// TODO
}

QueryResponse CTFBotMainAction::IsPositionAllowed(const INextBot *nextbot, const Vector& v1) const
{
	return QueryResponse::YES;
}

const CKnownEntity *CTFBotMainAction::SelectMoreDangerousThreat(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const
{
	CTFBot *actor = ToTFBot(nextbot->GetEntity());
	
	const CKnownEntity *result = this->SelectMoreDangerousThreatInternal(actor, them, threat1, threat2);
	
	bool target_healer_of_threat;
	switch (actor->m_iSkill) {
	
	// 0% chance to target healer of threat
	case CTFBot::DifficultyType::EASY:
		target_healer_of_threat = false;
		break;
	
	// 50% chance to target healer of threat
	case CTFBot::DifficultyType::NORMAL:
		target_healer_of_threat = (actor->TransientlyConsistentRandomValue(10.0f, 0) >= 0.5f);
		break;
	
	// 100% chance to target healer of threat
	case CTFBot::DifficultyType::HARD:
	case CTFBot::DifficultyType::EXPERT:
		target_healer_of_threat = true;
		break;
	}
	
	if (target_healer_of_threat) {
		return this->GetHealerOfThreat(result);
	} else {
		return result;
	}
}


void CTFBotMainAction::Dodge(CTFBot *actor)
{
	if (actor->m_iSkill == CTFBot::DifficultyType::EASY) {
		return;
	}
	
	if (actor->m_Shared.IsInvulnerable() ||
		actor->m_Shared.InCond(TF_COND_ZOOMED) ||
		actor->m_Shared.InCond(TF_COND_TAUNTING) ||
		(actor->m_nBotAttrs & CTFBot::AttributeType::DISABLEDODGE) != 0 ||
		actor->IsCombatWeapon()) {
		return;
	}
	
	if (actor->GetIntentionInterface()->ShouldHurry() == QueryResponse::YES ||
		actor->IsPlayerClass(TF_CLASS_ENGINEER) ||
		actor->m_Shared.InCond(TF_COND_DISGUISED) ||
		actor->m_Shared.InCond(TF_COND_DISGUISING) ||
		actor->m_Shared.IsStealthed()) {
		return;
	}
	
	const CKnownEntity *threat = this->GetVisionInterface()->GetPrimaryKnownThreat(false);
	if (threat == nullptr || !threat->IsVisibleRecently()) {
		return;
	}
	
	CTFWeaponBase *weapon = static_cast<CTFWeaponBase *>(actor->Weapon_GetSlot(0));
	if (weapon != nullptr && weapon->IsWeapon(TF_WEAPON_COMPOUND_BOW)) {
		CTFCompoundBow *huntsman = static_cast<CTFCompoundBow *>(weapon);
		if (huntsman->GetCurrentCharge() != 0.0f) {
			return;
		}
	} else {
		if (!this->IsLineOfFireClear(threat->GetLastKnownPosition())) {
			return;
		}
	}
	
	Vector eye_vec;
	actor->EyeVectors(&eye_vec);
	
	Vector2D side_dir = {
		.x = -eye_vec.y,
		.y =  eye_vec.x,
	};
	side_dir.NormalizeInPlace();
	
	int random = RandomInt(0, 100);
	if (random < 33) {
		Vector strafe_left = actor->GetAbsOrigin() + {
			.x = 25.0f * side_dir.x,
			.y = 25.0f * side_dir.y,
			.z = 0.0f,
		};
		
		if (!actor->GetLocomotionInterface()->HasPotentialGap(
			actor->GetAbsOrigin(), strafe_left, nullptr)) {
			actor->PressLeftButton();
		}
	} else if (rand > 66) {
		Vector strafe_right = actor->GetAbsOrigin() - {
			.x = 25.0f * side_dir.x,
			.y = 25.0f * side_dir.y,
			.z = 0.0f,
		};
		
		if (!actor->GetLocomotionInterface()->HasPotentialGap(
			actor->GetAbsOrigin(), strafe_right, nullptr)) {
			actor->PressRightButton();
		}
	}
}

void CTFBotMainAction::FireWeaponAtEnemy(CTFBot *actor)
{
	if (!actor->IsAlive()) {
		return;
	}
	
	if ((actor->m_nBotAttrs & (CTFBot::AttributeType::SUPPRESSFIRE |
		CTFBot::AttributeType::IGNOREENEMIES)) != 0) {
		return;
	}
	
	if (!tf_bot_fire_weapon_allowed.GetBool()) {
		return;
	}
	
	CTFWeaponBase *weapon = actor->m_Shared.GetActiveTFWeapon();
	if (weapon == nullptr) {
		return;
	}
	
	if (actor->IsBarrageAndReloadWeapon() && ((actor->m_nBotAttrs &
		CTFBot::AttributeType::HOLDFIREUNTILFULLRELOAD) != 0 ||
		tf_bot_always_full_reload.GetBool())) {
		if (weapon->Clip1() <= 0) {
			this->m_bReloadingBarrage = true;
		} else if (this->m_bReloadingBarrage) {
			if (weapon->Clip1() < weapon->GetMaxClip1()) {
				return;
			}
			
			this->m_bReloadingBarrage = false;
		}
	}
	
	if ((actor->m_nBotAttrs & CTFBot::AttributeType::ALWAYSFIREWEAPON) != 0) {
		actor->PressFireButton();
		return;
	}
	
	if (actor->IsPlayerClass(TF_CLASS_MEDIC) &&
		weapon->IsWeapon(TF_WEAPON_MEDIGUN)) {
		return;
	}
	
	if (actor->IsPlayerClass(TF_CLASS_HEAVYWEAPONS) && !actor->IsAmmoLow()) {
		if (actor->GetIntentionInterface()->ShouldHurry(actor) != QueryResponse::YES) {
			if (actor->GetVisionInterface()->GetTimeSinceVisible(GetEnemyTeam(actor)) < 3.0f) {
				actor->PressAltFireButton();
			}
		}
	}
	
	const CKnownEntity *threat = this->GetVisionInterface()->GetPrimaryKnownThreat(false);
	if (threat = nullptr || threat->GetEntity() == nullptr ||
		!threat->IsVisibleRecently()) {
		return;
	}
	
	if (!actor->IsLineOfFireClear(threat->GetEntity()->EyePosition()) &&
		!actor->IsLineOfFireClear(threat->GetEntity()->WorldSpaceCenter()) &&
		!actor->IsLineOfFireClear(threat->GetEntity()->GetAbsOrigin())) {
		return;
	}
	
	if (TFGameRules() != nullptr && TFGameRules()->IsMannVsMachineMode()) {
		CTFPlayer *player = ToTFPlayer(threat->GetEntity());
		if (player != nullptr && player->m_Shared.IsInvulnerable()) {
			/* BUG: incomplete list of weapon IDs
			 * (see CTFBot::IsExplosiveProjectileWeapon for details) */
			if (!weapon->IsWeapon(TF_WEAPON_ROCKETLAUNCHER) &&
				!weapon->IsWeapon(TF_WEAPON_GRENADELAUNCHER) &&
				!weapon->IsWeapon(TF_WEAPON_PIPEBOMBLAUNCHER) &&
				!weapon->IsWeapon(TF_WEAPON_DIRECTHIT)) {
				return;
			}
		}
	}
	
	if (!actor->GetIntentionInterface()->ShouldAttack(actor, threat) ||
		TFGameRules()->InSetup()) {
		return;
	}
	
	if (weapon->IsMeleeWeapon()) {
		if (actor->IsRangeLessThan(threat->GetEntity(), 250.0f)) {
			actor->PressFireButton();
		}
		
		return;
	}
	
	if (TFGameRules()->IsMannVsMachineMode()) {
		if (!actor->IsPlayerClass(TF_CLASS_SNIPER) &&
			weapon->IsHitScanWeapon() && actor->IsRangeGreaterThan(threat->GetEntity(),
			tf_bot_hitscan_range_limit.GetFloat()) {
			return;
		}
	}
	
	if (weapon->IsWeapon(TF_WEAPON_FLAMETHROWER)) {
		CTFFlameThrower *flamethrower = static_cast<CTFFlameThrower *>(weapon);
		if (flamethrower->CanAirBlast() && actor->ShouldFireCompressionBlast()) {
			actor->PressAltFireButton();
			return;
		}
		
		if (threat->GetTimeSinceLastSeen() < 1.0f) {
			Vector threat_to_actor = (actor->GetAbsOrigin() -
				threat->GetEntity()->GetAbsOrigin());
			if (threat_to_actor.IsLengthLessThan(actor->GetMaxAttackRange())) {
				actor->PressFireButton(tf_bot_fire_weapon_min_time.GetFloat());
			}
		}
		
		return;
	}
	
	Vector actor_to_threat = (threat->GetEntity()->GetAbsOrigin() -
		actor->GetAbsOrigin());
	float dist_to_threat = actor_to_threat.Length();
	
	if (!actor->GetBodyInterface()->IsHeadAimingOnTarget()) {
		return;
	}
	
	if (dist_to_threat >= actor->GetMaxAttackRange()) {
		return;
	}
	
	if (weapon->IsWeapon(TF_WEAPON_COMPOUND_BOW)) {
		CTFCompoundBow *huntsman = static_cast<CTFCompoundBow *>(weapon);
		if (huntsman->GetCurrentCharge() >= 0.95f &&
			actor->IsLineOfFireClear(threat->GetEntity())) {
			return;
		}
		
		actor->PressFireButton();
		return;
	}
	
	if (WeaponID_IsSniperRifle(weapon->GetWeaponID())) {
		if (!actor->m_Shared.InCond(TF_COND_ZOOMED)) {
			return;
		}
		
		// TODO: bunch of stuff related to IntervalTimer @ 0x5c
		// ...
		
		actor->PressFireButton();
		return;
	}
	
	if (!actor->IsCombatWeapon()) {
		return;
	}
	
	if (actor->IsContinuousFireWeapon()) {
		actor->PressFireButton(tf_bot_fire_weapon_min_time.GetFloat());
		return;
	}
	
	if (actor->IsExplosiveProjectileWeapon()) {
		Vector aim_vec;
		actor->EyeVectors(&aim_vec);
		aim_vec *= (1.1f * dist_to_threat);
		
		trace_t trace;
		UTIL_TraceLine(actor->EyePosition(),
			actor->EyePosition() + aim_vec,
			MASK_SHOT, actor, COLLISION_GROUP_NONE, &trace);
		
		if ((trace.fraction * (1.1f * dist_to_threat)) < 146.0f &&
			(trace.m_pEnt == nullptr || !trace.m_pEnt.IsCombatCharacter())) {
			return;
		}
	}
	
	actor->PressFireButton();
}

const CKnownEntity *CTFBotMainAction::GetHealerOfThreat(const CKnownEntity *threat) const
{
	if (threat == nullptr || threat->GetEntity() == nullptr) {
		return nullptr;
	}
	
	CTFPlayer *threat_player = ToTFPlayer(threat->GetEntity());
	if (threat_player == nullptr) {
		return threat;
	}
	
	const CKnownEntity *healer = threat;
	
	for (int i = 0; i < threat_player->m_Shared.m_nNumHealers; ++i) {
		CTFPlayer *healer_i = ToTFPlayer(threat_player->m_Shared.GetHealerByIndex(i));
		if (healer_i != nullptr) {
			healer = this->GetActor()->GetVisionInterface()->GetKnown(healer_i);
			if (healer != nullptr && healer->IsVisibleInFOVNow()) {
				break;
			}
		}
	}
	
	return healer;
}

bool CTFBotMainAction::IsImmediateThreat(const CBaseCombatCharacter *who, const CKnownEntity *threat) const
{
	CTFBot *actor = this->GetActor();
	if (actor == nullptr) {
		return false;
	}
	
	if (!actor->IsSelf(who)) {
		return false;
	}
	
	if (actor->InSameTeam(threat->GetEntity())) {
		return false;
	}
	
	if (!threat->GetEntity()->IsAlive()) {
		return false;
	}
	
	if (!threat->IsVisibleRecently()) {
		return false;
	}
	
	if (!actor->IsLineOfFireClear(threat->GetEntity())) {
		return false;
	}
	
	CTFPlayer *player = ToTFPlayer(threat->GetEntity());
	
	Vector threat_to_actor = actor->GetAbsOrigin() - threat->GetLastKnownPosition();
	float dist = threat_to_actor.Length();
	
	if (dist < 500.0f) {
		return true;
	}
	
	if (actor->IsThreatFiringAtMe(threat->GetEntity())) {
		return true;
	}
	
	threat_to_actor.NormalizeInPlace();
	
	Vector threat_eye_vec;
	
	if (player != nullptr) {
		if (player->IsPlayerClass(TF_CLASS_SNIPER)) {
			player->EyeVectors(&threat_eye_vec);
			return (threat_to_actor.Dot(threat_eye_vec) > 0.0f);
		}
		
		if (actor->m_iSkill > CTFBot::DifficultyType::NORMAL &&
			player->IsPlayerClass(TF_CLASS_MEDIC)) {
			return true;
		}
		
		if (actor->m_iSkill > CTFBot::DifficultyType::NORMAL &&
			player->IsPlayerClass(TF_CLASS_ENGINEER)) {
			return true;
		}
	} else {
		CBaseEntity *ent = threat->GetEntity();
		if (ent == nullptr) {
			return false;
		}
		
		CObjectSentrygun *sentry = dynamic_cast<CObjectSentrygun *>(ent);
		if (sentry != nullptr && !sentry->HasSapper() && !sentry->m_bPlacing && dist < 1650.0f) {
			AngleVectors(sentry->GetTurretAngles(), &threat_eye_vec);
			return (threat_to_actor.Dot(threat_eye_vec) > 0.8f);
		}
	}
	
	return false;
}

const CKnownEntity *CTFBotMainAction::SelectCloserThreat(CTFBot *actor, const CKnownEntity *threat1, const CKnownEntity *threat2) const
{
	float range1 = actor->GetRangeSquaredTo(known1->GetEntity());
	float range2 = actor->GetRangeSquaredTo(known2->GetEntity());
	
	if (range1 < range2) {
		return threat1;
	} else {
		return threat2;
	}
}

const CKnownEntity *CTFBotMainAction::SelectMoreDangerousThreatInternal(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const
{
	CTFBot *actor = ToTFBot(nextbot->GetEntity());
	
	const CKnownEntity *closer = this->SelectCloserThreat(actor, threat1, threat2);
	
	if ((actor->m_nRestrict & CTFBot::WeaponRestriction::MELEEONLY) != 0) {
		return closer;
	}
	
	if (!TFGameRules()->IsMannVsMachineMode()) {
		CObjectSentrygun *sentry1 = nullptr;
		if (threat1->IsVisibleRecently() && !threat1->IsPlayer()) {
			sentry1 = dynamic_cast<CObjectSentrygun *>(threat1->GetEntity());
		}
		CObjectSentrygun *sentry2 = nullptr;
		if (threat2->IsVisibleRecently() && !threat2->IsPlayer()) {
			sentry2 = dynamic_cast<CObjectSentrygun *>(threat2->GetEntity());
		}
		
		bool sentry1_danger =
			(sentry1 != nullptr && actor->IsRangeLessThan(sentry1, 1100.0f) &&
			!sentry1->HasSapper() && !sentry1->m_bPlacing());
		bool sentry2_danger =
			(sentry2 != nullptr && actor->IsRangeLessThan(sentry2, 1100.0f) &&
			!sentry2->HasSapper() && !sentry2->m_bPlacing());
		
		if (sentry1_danger && sentry2_danger) {
			return closer;
		} else if (sentry1_danger) {
			return threat1;
		} else if (sentry2_danger) {
			return threat2;
		}
	}
	
	if (TFGameRules()->IsMannVsMachineMode()) {
		const CKnownEntity *spy = SelectClosestSpyToMe(actor, threat1, threat2);
		if (spy != nullptr && actor->IsRangeLessThan(spy->GetEntity(), 1000.0f)) {
			return spy;
		}
	}
	
	bool imm1 = this->IsImmediateThreat(them, threat1);
	bool imm2 = this->IsImmediateThreat(them, threat2);
	
	if (imm1 && imm2) {
		const CKnownEntity *spy = SelectClosestSpyToMe(actor, threat1, threat2);
		if (spy != nullptr) {
			return spy;
		}
		
		bool firing1 = actor->IsThreatFiringAtMe(threat1->GetEntity());
		bool firing2 = actor->IsThreatFiringAtMe(threat2->GetEntity());
		
		if (firing1 && firing2) {
			return closer;
		} else if (firing1) {
			return threat1;
		} else if (firing2) {
			return threat2;
		} else {
			return closer;
		}
	} else if (imm1) {
		return threat1;
	} else if (imm2) {
		return threat2;
	} else {
		return closer;
	}
}


const CKnownEntity *SelectClosestSpyToMe(CTFBot *actor, const CKnownEntity *known1, const CKnownEntity *known2)
{
	CTFPlayer *spy1 = ToTFPlayer(known1->GetEntity());
	CTFPlayer *spy2 = ToTFPlayer(known2->GetEntity());
	
	bool spy1_valid = (spy1 != nullptr && spy1->IsPlayerClass(TF_CLASS_SPY));
	bool spy2_valid = (spy2 != nullptr && spy2->IsPlayerClass(TF_CLASS_SPY));
	
	if (spy1_valid && spy2_valid) {
		float range1 = actor->GetRangeSquaredTo(spy1);
		float range2 = actor->GetRangeSquaredTo(spy2);
		
		if (range1 > range2) {
			return known2;
		} else {
			return known1;
		}
	}
	
	if (spy1_valid) {
		return known1;
	}
	if (spy2_valid) {
		return known2;
	}
	
	return nullptr;
}
