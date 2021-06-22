/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/tf_bot.cpp
 */


ConVar tf_bot_force_class("tf_bot_force_class", "", FCVAR_GAMEDLL,
	"If set to a class name, all TFBots will respawn as that class");
ConVar tf_bot_notice_gunfire_range("tf_bot_notice_gunfire_range", "3000", FCVAR_GAMEDLL);
ConVar tf_bot_notice_quiet_gunfire_range("tf_bot_notice_quiet_gunfire_range", "500", FCVAR_GAMEDLL);
ConVar tf_bot_sniper_personal_space_range("tf_bot_sniper_personal_space_range", "1000", FCVAR_CHEAT);
ConVar tf_bot_pyro_deflect_tolerance("tf_bot_pyro_deflect_tolerance", "0.5", FCVAR_CHEAT);
ConVar tf_bot_keep_class_after_death("tf_bot_keep_class_after_death", "0", FCVAR_GAMEDLL);
ConVar tf_bot_prefix_name_with_difficulty("tf_bot_prefix_name_with_difficulty", "0", FCVAR_GAMEDLL,
	"Append the skill level of the bot to the bot's name");
ConVar tf_bot_near_point_travel_distance("tf_bot_near_point_travel_distance", "750", FCVAR_CHEAT);
ConVar tf_bot_pyro_shove_away_range("tf_bot_pyro_shove_away_range", "250", FCVAR_CHEAT,
	"If a Pyro bot's target is closer than this, compression blast them away");
ConVar tf_bot_pyro_always_reflect("tf_bot_pyro_always_reflect", "0", FCVAR_CHEAT,
	"Pyro bots will always reflect projectiles fired at them. For tesing/debugging purposes.");
ConVar tf_bot_sniper_spot_min_range("tf_bot_sniper_spot_min_range", "1000", FCVAR_CHEAT);
ConVar tf_bot_sniper_spot_max_count("tf_bot_sniper_spot_max_count", "10", FCVAR_CHEAT,
	"Stop searching for sniper spots when each side has found this many");
ConVar tf_bot_sniper_spot_search_count("tf_bot_sniper_spot_search_count", "10", FCVAR_CHEAT,
	"Search this many times per behavior update frame");
ConVar tf_bot_sniper_spot_point_tolerance("tf_bot_sniper_spot_point_tolerance", "750", FCVAR_CHEAT);
ConVar tf_bot_sniper_spot_epsilon("tf_bot_sniper_spot_epsilon", "100", FCVAR_CHEAT);
ConVar tf_bot_sniper_goal_entity_move_tolerance("tf_bot_sniper_goal_entity_move_tolerance", "500", FCVAR_CHEAT);
ConVar tf_bot_suspect_spy_touch_interval("tf_bot_suspect_spy_touch_interval", "5", FCVAR_CHEAT,
	"How many seconds back to look for touches against suspicious spies");
ConVar tf_bot_suspect_spy_forced_cooldown("tf_bot_suspect_spy_forced_cooldown", "5", FCVAR_CHEAT,
	"How long to consider a suspicious spy as suspicious");
ConVar tf_bot_debug_tags("tf_bot_debug_tags", "0", FCVAR_CHEAT,
	"ent_text will only show tags on bots");

ConCommand tf_bot_add_command("tf_bot_add", &tf_bot_add,
	"Add a bot.", FCVAR_GAMEDLL);
ConCommand tf_bot_kick_command("tf_bot_kick", &tf_bot_kick,
	"Remove a TFBot by name, or all bots (\"all\").", FCVAR_GAMEDLL);
ConCommand tf_bot_kill_command("tf_bot_kill", &tf_bot_kill,
	"Kill a TFBot by name, or all bots (\"all\").", FCVAR_GAMEDLL);
ConCommand tf_bot_warp_team_to_me("tf_bot_warp_team_to_me", &CMD_BotWarpTeamToMe,
	"", FCVAR_CHEAT | FCVAR_GAMEDLL);


CTFBotPathCost::CTFBotPathCost(CTFBot *actor, RouteType rtype)
	: m_Actor(actor), m_iRouteType(rtype)
{
	this->m_flStepHeight      = actor->GetLocomotionInterface()->GetStepHeight();
	this->m_flMaxJumpHeight   = actor->GetLocomotionInterface()->GetMaxJumpHeight();
	this->m_flDeathDropHeight = actor->GetLocomotionInterface()->GetDeathDropHeight();
	
	if (actor->IsPlayerClass(TF_CLASS_SPY)) {
		TheNavMesh->CollectBuiltObjects(&this->m_Objects, GetEnemyTeam(actor));
	} else {
		this->m_Objects.RemoveAll();
	}
}

float CTFBotPathCost::operator()(CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length) const
{
	VPROF_BUDGET("CTFBotPathCost::operator()", "NextBot");
	
	CTFNavArea *tfArea = static_cast<CTFNavArea *>(area);
	
	if (fromArea == nullptr) {
		/* first area in path; zero cost */
		return 0.0f;
	}
	
	if (!this->m_Actor->GetLocomotionInterface()->IsAreaTraversable(area)) {
		/* dead end */
		return -1.0f;
	}
	
	if (TFGameRules()->IsInTraining() && tfArea->HasTFAttributes(CONTROL_POINT) && !this->m_Actor->IsAnyPointBeingCaptured() && !this->m_Actor->IsPlayClass(TF_CLASS_ENGINEER)) {
		/* dead end */
		return -1.0f;
	}
	
	if ((this->m_Actor->GetTeamNumber() == TF_TEAM_RED && tfArea->HasTFAttributes(BLUE_SPAWN_ROOM)) ||
		(this->m_Actor->GetTeamNumber() == TF_TEAM_BLUE && tfArea->HasTFAttributes(RED_SPAWN_ROOM))) {
		if (TeamplayRoundBasedRules()->State_Get() != GR_STATE_TEAM_WIN) {
			/* dead end */
			return -1.0f;
		}
	}
	
	float dist;
	if (ladder != nullptr) {
		dist = ladder->m_length;
	} else if (length != 0.0f) {
		dist = length;
	} else {
		dist = (area->GetCenter() - fromArea->GetCenter()).Length();
	}
	
	float delta_z = fromArea->ComputeAdjacentConnectionHeightChange(area);
	if (delta_z >= this->m_flStepHeight) {
		if (delta_z >= this->m_flMaxJumpHeight) {
			return -1.0f;
		}
		
		/* x2 distance penalty for going up steps */
		dist *= 2;
	} else {
		if (delta_z < -this->m_flDeathDropHeight) {
			return -1.0f;
		}
	}
	
	float multiplier = 1.0f;
	
	if (this->m_iRouteType == DEFAULT_ROUTE) {
		if (!this->m_Actor->IsMiniBoss()) {
			/* very similar to CTFBot::TransientlyConsistentRandomValue */
			int seed = (int)(gpGlobals->curtime * 0.1f) + 1;
			seed *= area->GetID();
			seed *= ENTINDEX(this);
			
			/* huge random cost modifier [0, 100] for non-giant bots! */
			multiplier += (pfFastCos((float)seed) + 1.0f) * 50.0f;
		}
	}
	
	if (this->m_iRouteType == SAFEST_ROUTE) {
		if (tfArea->IsInCombat()) {
			dist *= 4.0f * tfArea->GetCombatIntensity();
		}
		
		if ((this->m_Actor->GetTeamNumber() == TF_TEAM_RED && tfArea->HasTFAttributes(BLUE_SENTRY)) ||
			(this->m_Actor->GetTeamNumber() == TF_TEAM_BLUE && tfArea->HasTFAttributes(RED_SENTRY))) {
			dist *= 5.0f;
		}
	}
	
	/* BUG: why are we not using the list of objects we collected in the ctor? */
	if (this->m_Actor->IsPlayerClass(TF_CLASS_SPY)) {
		int enemy_team = GetEnemyTeam(this->m_Actor);
		
		for (int i = 0; i < IBaseObjectAutoList::AutoList().Count(); ++i) {
			CBaseObject *obj = static_cast<CBaseObject *>(IBaseObjectAutoList::AutoList()[i]);
			
			if (obj->GetType() == OBJ_SENTRYGUN && obj->GetTeamNumber() == enemy_team) {
				obj->UpdateLastKnownArea();
				if (area == obj->GetLastKnownArea()) {
					dist *= 10.0f;
				}
			}
		}
		
		dist += (dist * 10.0f * area->GetPlayerCount(this->m_Actor->GetTeamNumber()));
	}
	
	float cost = dist * multiplier;
	
	if (area->HasAttributes(NAV_MESH_FUNC_COST)) {
		cost *= area->ComputeFuncNavCost(this->m_Actor);
	}
	
	return fromArea->GetCostSoFar() + cost;
}


CTFBot::CTFBotIntention::CTFBotIntention(CTFBot *actor)
	: IIntention(actor)
{
	// TODO: verify parameters for CTFBotMainAction ctor
	this->m_Behavior = new Behavior<CTFBot>(new CTFBotMainAction());
}

CTFBot::CTFBotIntention::~CTFBotIntention()
{
	if (this->m_Behavior != nullptr) {
		delete this->m_Behavior;
	}
}


INextBotEventResponder *CTFBot::CTFBotIntention::FirstContainedResponder() const
{
	return this->m_Behavior;
}

INextBotEventResponder *CTFBot::CTFBotIntention::NextContainedResponder(INextBotEventResponder *prev) const
{
	return nullptr;
}


void CTFBot::CTFBotIntention::Reset()
{
	if (this->m_Behavior != nullptr) {
		delete this->m_Behavior;
	}
	
	// TODO: verify parameters for CTFBotMainAction ctor
	this->m_Behavior = new Behavior<CTFBot>(new CTFBotMainAction());
}

void CTFBot::CTFBotIntention::Update()
{
	this->m_Behavior->Update(this->GetBot(), this->m_flTickInterval);
}


float CTFBot::TransientlyConsistentRandomValue(float duration, int seed) const
{
	CNavArea *area = this->GetLastKnownArea();
	if (area == nullptr) {
		return 0.0f;
	}
	
	int time_seed = (int)(gpGlobals->curtime / duration) + 1;
	seed += (area->GetID() * time_seed * ENTINDEX(this));
	
	return Abs(pfFastCos((float)seed));
}


bool CTFBot::IsWeaponRestricted(CTFWeaponBase *weapon) const
{
	// TODO
}


bool CTFBot::IsCombatWeapon(CTFWeaponBase *weapon) const
{
	if (weapon == nullptr) {
		weapon = this->m_Shared.GetActiveTFWeapon();
		if (weapon == nullptr) {
			return true;
		}
	}
	
	/* BUG: this function is horribly outdated; post-2010 weapons are all
	 * considered combat weapons by default
	 */
	
	switch (weapon->GetWeaponID()) {
	case TF_WEAPON_PDA:
	case TF_WEAPON_PDA_ENGINEER_BUILD:
	case TF_WEAPON_PDA_ENGINEER_DESTROY:
	case TF_WEAPON_PDA_SPY:
	case TF_WEAPON_BUILDER:
	case TF_WEAPON_MEDIGUN:
	case TF_WEAPON_DISPENSER:
	case TF_WEAPON_INVIS:
	case TF_WEAPON_LUNCHBOX:
	case TF_WEAPON_BUFF_ITEM:
	case TF_WEAPON_PUMPKIN_BOMB:
		return false;
		
	default:
		return true;
	}
}

bool CTFBot::IsQuietWeapon(CTFWeaponBase *weapon) const
{
	if (weapon == nullptr) {
		weapon = this->m_Shared.GetActiveTFWeapon();
		if (weapon == nullptr) {
			return false;
		}
	}
	
	/* BUG: this function is horribly outdated; post-2010 weapons are all
	 * considered non-quiet weapons by default
	 * also: why is jarate quiet but milk isn't?
	 */
	
	switch (weapon->GetWeaponID()) {
	case TF_WEAPON_KNIFE:
	case TF_WEAPON_FISTS:
	case TF_WEAPON_PDA:
	case TF_WEAPON_PDA_ENGINEER_BUILD:
	case TF_WEAPON_PDA_ENGINEER_DESTROY:
	case TF_WEAPON_PDA_SPY:
	case TF_WEAPON_BUILDER:
	case TF_WEAPON_MEDIGUN:
	case TF_WEAPON_DISPENSER:
	case TF_WEAPON_INVIS:
	case TF_WEAPON_FLAREGUN:
	case TF_WEAPON_LUNCHBOX:
	case TF_WEAPON_JAR:
	case TF_WEAPON_COMPOUND_BOW:
	case TF_WEAPON_SWORD:
	case TF_WEAPON_CROSSBOW:
		return true;
		
	default:
		return false;
	}
}


bool CTFBot::IsHitScanWeapon(CTFWeaponBase *weapon) const
{
	if (weapon == nullptr) {
		weapon = this->m_Shared.GetActiveTFWeapon();
		if (weapon == nullptr) {
			return false;
		}
	}
	
	switch (weapon->GetWeaponID()) {
	case TF_WEAPON_SHOTGUN_PRIMARY:
	case TF_WEAPON_SHOTGUN_SOLDIER:
	case TF_WEAPON_SHOTGUN_HWG:
	case TF_WEAPON_SHOTGUN_PYRO:
	case TF_WEAPON_SCATTERGUN:
	case TF_WEAPON_SNIPERRIFLE:
	case TF_WEAPON_MINIGUN:
	case TF_WEAPON_SMG:
	case TF_WEAPON_PISTOL:
	case TF_WEAPON_PISTOL_SCOUT:
	case TF_WEAPON_REVOLVER:
	case TF_WEAPON_SENTRY_BULLET:
	case TF_WEAPON_SENTRY_ROCKET:
	case TF_WEAPON_SENTRY_REVENGE:
	case TF_WEAPON_HANDGUN_SCOUT_PRIMARY:
	case TF_WEAPON_HANDGUN_SCOUT_SEC:
	case TF_WEAPON_SODA_POPPER:
	case TF_WEAPON_SNIPERRIFLE_DECAP:
	case TF_WEAPON_PEP_BRAWLER_BLASTER:
	case TF_WEAPON_SNIPERRIFLE_CLASSIC:
		return true;
		
	default:
		return false;
	}
}

bool CTFBot::IsExplosiveProjectileWeapon(CTFWeaponBase *weapon) const
{
	if (weapon == nullptr) {
		weapon = this->m_Shared.GetActiveTFWeapon();
		if (weapon == nullptr) {
			return false;
		}
	}
	
	/* BUG: this function is horribly outdated; post-2010 weapons are all
	 * considered non-explosive weapons by default
	 * TF_WEAPON_PARTICLE_CANNON is conspicuously absent (Cow Mangler)
	 * TF_WEAPON_CANNON is conspicuously absent (Loose Cannon)
	 * 
	 * surely they could implement this function as a dynamic_cast or something
	 * instead of a weapon ID whitelist that has to be updated constantly?
	 */
	
	switch (weapon->GetWeaponID()) {
	case TF_WEAPON_ROCKETLAUNCHER:
	case TF_WEAPON_GRENADELAUNCHER:
	case TF_WEAPON_PIPEBOMBLAUNCHER:
	case TF_WEAPON_JAR:
	case TF_WEAPON_DIRECTHIT:
		return true;
		
	default:
		return false;
	}
}


bool CTFBot::IsContinuousFireWeapon(CTFWeaponBase *weapon) const
{
	if (weapon == nullptr) {
		weapon = this->m_Shared.GetActiveTFWeapon();
	}
	
	if (!this->IsCombatWeapon(weapon)) {
		return false;
	}
	
	if (weapon == nullptr) {
		return true;
	}
	
	switch (weapon->GetWeaponID()) {
	case TF_WEAPON_ROCKETLAUNCHER:
	case TF_WEAPON_GRENADELAUNCHER:
	case TF_WEAPON_PIPEBOMBLAUNCHER:
	case TF_WEAPON_PISTOL:
	case TF_WEAPON_PISTOL_SCOUT:
	case TF_WEAPON_FLAREGUN:
	case TF_WEAPON_JAR:
	case TF_WEAPON_COMPOUND_BOW:
	case TF_WEAPON_DIRECTHIT:
		return false;
		
	default:
		return true;
	}
}

bool CTFBot::IsBarrageAndReloadWeapon(CTFWeaponBase *weapon) const
{
	if (weapon == nullptr) {
		weapon = this->m_Shared.GetActiveTFWeapon();
		if (weapon == nullptr) {
			return false;
		}
	}
	
	/* BUG: this function is horribly outdated; post-2010 weapons are all
	 * considered non-barrage weapons by default
	 * TF_WEAPON_PARTICLE_CANNON is conspicuously absent (Cow Mangler)
	 * TF_WEAPON_CANNON is conspicuously absent (Loose Cannon)
	 * TF_WEAPON_SODA_POPPER is conspicuously absent (Soda Popper)
	 * TF_WEAPON_PEP_BRAWLER_BLASTER is conspicuously absent (BFB)
	 * 
	 * surely they could implement this function as a dynamic_cast or something
	 * instead of a weapon ID whitelist that has to be updated constantly?
	 */
	
	switch (weapon->GetWeaponID()) {
	case TF_WEAPON_SCATTERGUN:
	case TF_WEAPON_ROCKETLAUNCHER:
	case TF_WEAPON_GRENADELAUNCHER:
	case TF_WEAPON_PIPEBOMBLAUNCHER:
	case TF_WEAPON_DIRECTHIT:
		return true;
		
	default:
		return false;
	}
}


bool CTFBot::IsAmmoLow(CTFWeaponBase *weapon) const
{
	// TODO
}

bool CTFBot::IsAmmoFull(CTFWeaponBase *weapon) const
{
	// TODO
}


float CTFBot::GetMaxAttackRange() const
{
	CTFWeaponBase *weapon = this->m_Shared.GetActiveTFWeapon();
	if (weapon == nullptr) {
		return 0.0f;
	}
	
	if (weapon->IsMeleeWeapon()) {
		return 100.0f;
	}
	
	if (weapon->IsWeapon(TF_WEAPON_FLAMETHROWER)) {
		if (TFGameRules()->IsMannVsMachineMode()) {
			static CSchemaFieldHandle<CEconItemDefinition>
				pItemDef_GiantFlamethrower("MVM Giant Flamethrower");
			
			if (this->IsActiveTFWeapon(pItemDef_GiantFlamethrower)) {
				return 875.0f;
			} else {
				return 350.0f;
			}
		} else {
			return 250.0f;
		}
	}
	
	if (WeaponID_IsSniperRifle(weapon->GetWeaponID())) {
		return FLT_MAX;
	}
	
	if (weapon->IsWeapon(TF_WEAPON_ROCKETLAUNCHER)) {
		return 3000.0f;
	}
	
	return FLT_MAX;
}


void CTFBot::DisguiseAsMemberOfEnemyTeam()
{
	CUtlVector<CTFPlayer *> enemies;
	CollectPlayers<CTFPlayer>(&enemies, GetEnemyTeam(this), false, false);
	
	int classnum;
	if (!enemies.IsEmpty()) {
		classnum = enemies.Random()->GetPlayerClass()->GetClassIndex();
	} else {
		classnum = RandomInt(1, 9);
	}
	
	this->m_Shared.Disguise(GetEnemyTeam(this), classnum, false, false);
}


Action<CTFBot> *CTFBot::OpportunisticallyUseWeaponAbilities()
{
	if (!this->m_ctUseWeaponAbilities.IsElapsed()) {
		return nullptr;
	}
	this->m_ctUseWeaponAbilities.Start(RandomFloat(0.1f, 0.2f));
	
	if (this->IsPlayerClass(TF_CLASS_DEMOMAN) && this->m_Shared.m_bShieldEquipped) {
		Vector eye_vec;
		this->EyeVectors(&eye_vec);
		
		if (this->GetLocomotionInterface()->IsPotentiallyTraversible(
			this->GetAbsOrigin(), this->GetAbsOrigin() + (100.0f * eye_vec),
			(ILocomotion::TraverseWhenType)0, nullptr)) {
			if ((this->m_nBotAttrs & CTFBot::AttributeType::AIRCHARGEONLY) != 0) {
				if (this->GetGroundEntity() == nullptr &&
					this->GetAbsVelocity()->z <= 0.0f) {
					this->PressAltFireButton();
				}
			} else {
				this->PressAltFireButton();
			}
		}
	}
	
	for (int i = 0; i < MAX_WEAPONS; ++i) {
		CBaseCombatWeapon *weapon = player->GetWeapon(i);
		if (weapon == nullptr) {
			continue;
		}
		
		if (weapon->GetWeaponID() == TF_WEAPON_BUFF_ITEM) {
			CTFBuffItem *buff = static_cast<CTFBuffItem *>(weapon);
			if (buff->IsFull()) {
				return new CTFBotUseItem(weapon);
			}
			
			continue;
		}
		
		if (weapon->GetWeaponID() == TF_WEAPON_LUNCHBOX) {
			if (!weapon->HasAmmo() || (this->IsPlayerClass(TF_CLASS_SCOUT) &&
				this->m_Shared.m_flEnergyDrinkMeter < 100.0f)) {
				continue;
			}
			
			return new CTFBotUseItem(weapon);
		}
		
		if (weapon->GetWeaponID() == TF_WEAPON_BAT_WOOD &&
			this->GetAmmoCount(TF_AMMO_GRENADES1) > 0) {
			const CKnownEntity *threat = this->GetVisionInterface()->
				GetPrimaryKnownThreat(false);
			if (threat == nullptr || !threat->IsVisibleRecently()) {
				continue;
			}
			
			this->PressAltFireButton();
		}
	}
	
	return nullptr;
}

bool CTFBot::IsLineOfFireClear(CBaseEntity *to) const
{
	return this->IsLineOfFireClear(this->EyePosition(), to);
}

bool CTFBot::IsLineOfFireClear(const Vector& to) const
{
	return this->IsLineOfFireClear(this->EyePosition(), to);
}

bool CTFBot::IsLineOfFireClear(const Vector& from, CBaseEntity *to) const
{
	NextBotTraceFilterIgnoreActors filter(nullptr);
	
	trace_t trace;
	UTIL_TraceLine(from, to->WorldSpaceCenter(),
		MASK_SOLID_BRUSHONLY, &filter, &trace);
	
	if (trace.traction >= 1.0f && !trace.allsolid) {
		return (!trace.startsolid || trace.m_pEnt == to);
	}
	
	return false;
}

bool CTFBot::IsLineOfFireClear(const Vector& from, const Vector& to) const
{
	NextBotTraceFilterIgnoreActors filter(nullptr);
	
	trace_t trace;
	UTIL_TraceLine(from, to, MASK_SOLID_BRUSHONLY, &filter, &trace);
	
	if (trace.fraction >= 1.0f && !trace.allsolid) {
		return (!trace.startsolid);
	}
	
	return false;
}


SuspectedSpyInfo_t *CTFBot::IsSuspectedSpy(CTFPlayer *spy)
{
	FOR_EACH_VEC(this->m_SuspectedSpies, i) {
		SuspectedSpyInfo_t *info = this->m_SuspectedSpies[i];
		
		CTFPlayer *spy2 = info->m_hSpy;
		if (spy2 != nullptr) {
			if (ENTINDEX(spy2) == ENTINDEX(spy)) {
				return info;
			}
		}
	}
	
	return nullptr;
}

void CTFBot::SuspectSpy(CTFPlayer *spy)
{
	SuspectedSpyInfo_t *info = this->IsSuspectedSpy(spy);
	if (info == nullptr) {
		info = new SuspectedSpyInfo_t(spy);
		this->m_SuspectedSpies.AddToHead(info);
	}
	
	info->Suspect();
	if (info->TestForRealizing()) {
		this->RealizeSpy(spy);
	}
}

void CTFBot::StopSuspectingSpy(CTFPlayer *spy)
{
	FOR_EACH_VEC(this->m_SuspectedSpies, i) {
		SuspectedSpyInfo_t *info = this->m_SuspectedSpies[i];
		
		CTFPlayer *spy2 = info->m_hSpy;
		if (spy2 != nullptr) {
			if (ENTINDEX(spy2) == ENTINDEX(spy)) {
				delete info;
				this->m_SuspectedSpies.Remove(i);
				return;
			}
		}
	}
}


bool CTFBot::IsKnownSpy(CTFPlayer *spy) const
{
	return this->m_KnownSpies.HasElement(spy);
}

void CTFBot::RealizeSpy(CTFPlayer *spy)
{
	if (this->IsKnownSpy(spy)) {
		return;
	}
	this->m_KnownSpies.AddToHead(spy);
	
	this->SpeakConceptIfAllowed(MP_CONCEPT_PLAYER_CLOAKEDSPY);
	
	SuspectedSpyInfo_t *info = this->IsSuspectedSpy(spy);
	if (info != nullptr && info->IsCurrentlySuspected()) {
		CUtlVector<CTFPlayer *> teammates;
		CollectPlayers<CTFPlayer>(&teammates, this->GetTeamNumber(), true, false);
		
		FOR_EACH_VEC(teammates, i) {
			/* BUG: IsBot() == true is insufficient to establish that a
			 * CTFPlayer is a CTFBot; they may just be a puppet bot, in which
			 * case this could potentially cause a crash */
			if (teammates[i]->IsBot()) {
				CTFBot *teammate = static_cast<CTFBot *>(teammates[i]);
				
				if (this->EyePosition().DistToSqr(teammate->EyePosition()) < (512.0f * 512.0f)) {
					if (!teammate->IsKnownSpy(spy)) {
						teammate->SuspectSpy(spy);
						teammate->RealizeSpy(spy);
					}
				}
			}
		}
	}
}

void CTFBot::ForgetSpy(CTFPlayer *spy)
{
	this->StopSuspectingSpy(spy);
	this->m_KnownSpies.FindAndFastRemove(spy);
}


void CTFBot::DelayedThreatNotice(CHandle<CBaseEntity> ent, float delay)
{
	float when = gpGlobals->curtime + delay;
	
	FOR_EACH_VEC(this->m_DelayedThreatNotices, i) {
		if (ent.Get() == this->m_DelayedThreatNotices[i].Get()) {
			if (when < info->m_flWhen) {
				info->m_flWhen = when;
			}
			return;
		}
	}
	
	this->m_DelayedThreatNotices.AddToTail({ ent, delay });
}

void CTFBot::UpdateDelayedThreatNotices()
{
	FOR_EACH_VEC(this->m_DelayedThreatNotices, i) {
		DelayedNoticeInfo *info = this->m_DelayedThreatNotices[i];
		
		if (gpGlobals->curtime >= info->m_flWhen) {
			CBaseEntity *ent = info->m_hEnt;
			if (ent != nullptr) {
				CTFPlayer *player = ToTFPlayer(ent);
				if (player != nullptr && player->IsPlayerClass(TF_CLASS_SPY)) {
					this->RealizeSpy(player);
				}
				
				this->GetVisionInterface()->AddKnownEntity(ent);
			}
			
			/* BUG: removing while in the loop means we'll skip a notice */
			this->m_DelayedThreatNotices.Remove(i);
		}
	}
}
