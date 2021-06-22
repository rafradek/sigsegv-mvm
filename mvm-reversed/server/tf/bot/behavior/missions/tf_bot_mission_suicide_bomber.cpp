/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/missions/tf_bot_mission_suicide_bomber.cpp
 * used in MvM: TODO
 */


ConVar tf_bot_suicide_bomb_range("tf_bot_suicide_bomb_range", "300", FCVAR_CHEAT);
ConVar tf_bot_suicide_bomb_friendly_fire("tf_bot_suicide_bomb_friendly_fire", "1", FCVAR_CHEAT);


CTFBotMissionSuicideBomber::CTFBotMissionSuicideBomber()
{
}

CTFBotMissionSuicideBomber::~CTFBotMissionSuicideBomber()
{
}

const char *CTFBotMissionSuicideBomber::GetName() const
{
	return "MissionSuicideBomber";
}


ActionResult<CTFBot> CTFBotMissionSuicideBomber::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_bDetonating       = false;
	this->m_bDetReachedGoal   = false;
	this->m_bDetLostAllHealth = false;
	
	this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
	
	this->m_ctDetonation->Invalidate();
	this->m_nConsecutivePathFailures = 0;
	
	if (actor->m_hSBTarget() != nullptr) {
		CBaseEntity *target = actor->m_hSBTarget();
		this->m_hTarget = target;
		
		if (target != nullptr) {
			this->m_vecTargetPos = target->GetAbsOrigin();
		}
	} else {
		this->m_hTarget = nullptr;
	}
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotMissionSuicideBomber::Update(CTFBot *actor, float dt)
{
	if (this->m_ctDetonation.HasStarted()) {
		if (!this->m_ctDetonation.IsElapsed()) {
			return ActionResult<CTFBot>::Continue();
		}
		
		this->m_vecDetonatePos = actor->GetAbsOrigin();
		this->Detonate(actor);
		
		if (this->m_bDetReachedGoal && this->m_hTarget() != nullptr &&
			this->m_hTarget()->IsBaseObject()) {
			CObjectSentrygun *sentry =
				dynamic_cast<CObjectSentrygun *>(this->m_hTarget());
			if (sentry != nullptr && sentry->GetOwner() != nullptr) {
				CTFPlayer *owner = ToTFPlayer(sentry->GetOwner());
				if (owner != nullptr) {
					CGameEvent *event = gameeventmanager->CreateEvent("mvm_sentrybuster_detonate");
					if (event != nullptr) {
						event->SetInt("player", ENTINDEX(owner));
						event->SetFloat("det_x", this->m_vecDetonatePos.x);
						event->SetFloat("det_y", this->m_vecDetonatePos.y);
						event->SetFloat("det_z", this->m_vecDetonatePos.z);
						gameeventmanager->FireEvent(event);
					}
				}
			}
		}
		
		return Done("KABOOM!");
	}
	
	if (actor->GetHealth() == 1) {
		this->StartDetonate(actor, false, true);
		return ActionResult<CTFBot>::Continue();
	}
	
	if (this->m_hTarget() != nullptr) {
		if (this->m_hTarget()->IsAlive() && !this->m_hTarget()->IsDying()) {
			this->m_vecTargetPos = this->m_hTarget()->GetAbsOrigin();
		}
		
		if (this->m_hTarget()->IsBaseObject()) {
			CObjectSentrygun *sentry =
				dynamic_cast<CObjectSentrygun *>(this->m_hTarget());
			if (sentry != nullptr &&
				sentry->m_bCarried && sentry->GetOwner() != nullptr) {
				this->m_vecTargetPos = sentry->GetOwner()->GetAbsOrigin();
			}
		}
	}
	
	float goal_range = (1.0f / 3.0f) * tf_bot_suicide_bomb_range.GetFloat();
	
	if (this->m_vecTargetPos.DistToSqr(actor->GetAbsOrigin()) <
		(goal_range * goal_range) && actor->IsLineOfFireClear({
			.x = this->m_vecTargetPos.x,
			.y = this->m_vecTargetPos.y,
			.z = this->m_vecTargetPos.z + 18.0f,
		})) {
		this->StartDetonate(actor, true, false);
	}
	
	if (this->m_ctPlaySound.IsElapsed()) {
		this->m_ctPlaySound.Start(4.0f);
		actor->EmitSound("MVM.SentryBusterIntro");
	}
	
	if (this->m_ctRecomputePath.IsElapsed()) {
		this->m_ctRecomputePath.Start(RandomFloat(0.5f, 1.0f));
		
		CTFBotPathCost cost_func(actor, FASTEST_ROUTE);
		if (this->m_PathFollower.Compute<CTFBotPathCost>(actor,
			this->m_vecTargetPos, cost_func, 0.0f, true)) {
			this->m_nConsecutivePathFailures = 0;
		} else {
			if (++this->m_nConsecutivePathFailures > 2) {
				this->StartDetonate(actor, false, false);
			}
		}
	}
	
	this->m_PathFollower.Update(actor);
	return ActionResult<CTFBot>::Continue();
}

void CTFBotMissionSuicideBomber::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
}


EventDesiredResult<CTFBot> CTFBotMissionSuicideBomber::OnStuck(CTFBot *actor)
{
	if (!this->m_bDetonating && !this->m_ctDetonation.HasStarted()) {
		this->StartDetonate(actor, false, false);
	}
	
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotMissionSuicideBomber::OnKilled(CTFBot *actor, const CTakeDamageInfo& info)
{
	/* how we get here:
	 * CBaseCombatCharacter::Event_Killed
	 * NextBotManager::OnKilled
	 * Action<CTFBot>::OnKilled
	 * CTFBotMissionSuicideBomber::OnKilled
	 */
	
	if (!this->m_bDetonating) {
		if (!this->m_ctDetonation.HasStarted()) {
			this->StartDetonate(actor, false, false);
		} else {
			/* BUG: probably bad to call Detonate when m_bDetonating is false
			 * and we haven't called StartDetonate... */
			if (this->m_ctDetonation.IsElapsed()) {
				this->Detonate(actor);
			} else {
				if (actor->GetTeamNumber() != TEAM_SPECTATOR) {
					actor->m_lifeState = LIFE_ALIVE;
					actor->SetHealth(1);
				}
			}
		}
	}
	
	return EventDesiredResult<CTFBot>::Continue();
}


QueryResponse CTFBotMissionSuicideBomber::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	return QueryResponse::NO;
}


void CTFBotMissionSuicideBomber::StartDetonate(CTFBot *actor, bool reached_goal, bool lost_all_health)
{
	if (this->m_ctDetonation.HasStarted()) {
		return;
	}
	
	if ((!actor->IsAlive() || actor->GetHealth() <= 0) &&
		actor->GetTeamNumber() != TEAM_SPECTATOR) {
		actor->m_lifeState = LIFE_ALIVE;
		actor->SetHealth(1);
	}
	
	this->m_bDetReachedGoal   = reached_goal;
	this->m_bDetLostAllHealth = lost_all_health;
	
	actor->m_takedamage = DAMAGE_NO;
	
	// TODO: enum/default values for CTFPlayer::Taunt(taunts_t, int)
	actor->Taunt(0, 0);
	
	this->m_ctDetonation.Start(2.0f);
	
	actor->EmitSound("MvM.SentryBusterSpin");
}

void CTFBotMissionSuicideBomber::Detonate(CTFBot *actor)
{
	this->m_bDetonating = true;
	
	DispatchParticleEffect("explosionTrail_seeds_mvm",
		actor->GetAbsOrigin(), actor->GetAbsAngles());
	DispatchParticleEffect("fluidSmokeExpl_ring_mvm",
		actor->GetAbsOrigin(), actor->GetAbsAngles());
	
	actor->EmitSound("MvM.SentryBusterExplode");
	
	UTIL_ScreenShake(actor->GetAbsOrigin(),
		25.0f, 5.0f, 5.0f, 1000.0f, SHAKE_START);
	
	if (!this->m_bDetReachedGoal && TFGameRules() != nullptr &&
		TFGameRules()->IsMannVsMachineMode()) {
		TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed(
			TLK_MVM_SENTRY_BUSTER_DOWN, TF_TEAM_RED);
		
		for (int i = 0; i < 4; ++i) {
			CTFPlayer *damager = TFGameRules()->GetRecentDamager(actor, i, 5.0f);
			if (damager != nullptr) {
				damager->AwardAchievement(TF_MVM_KILL_SENTRY_BUSTER);
			}
		}
	}
	
	CUtlVector<CTFPlayer *> players_bothteams;
	CollectPlayers<CTFPlayer>(&players_bothteams, TF_TEAM_RED, true, false);
	CollectPlayers<CTFPlayer>(&players_bothteams, TF_TEAM_BLUE, true, true);
	
	CUtlVector<CBaseCombatCharacter *> potential_victims;
	FOR_EACH_VEC(players_bothteams, i) {
		CTFPlayer *player = players_bothteams[i];
		potential_victims.AddToTail(static_cast<CBaseCombatCharacter *>(player));
	}
	
	CTFTeam *team_blu = GetGlobalTFTeam(TF_TEAM_BLUE);
	if (team_blu != nullptr) {
		for (int i = 0; i < team_blu->GetNumObjects(); ++i) {
			CBaseObject *obj = team_blu->GetObject(i);
			if (obj != nullptr) {
				potential_victims.AddToTail(static_cast<CBaseCombatCharacter *>(obj));
			}
		}
	}
	
	CTFTeam *team_red = GetGlobalTFTeam(TF_TEAM_RED);
	if (team_red != nullptr) {
		for (int i = 0; i < team_red->GetNumObjects(); ++i) {
			CBaseObject *obj = team_red->GetObject(i);
			if (obj != nullptr) {
				potential_victims.AddToTail(static_cast<CBaseCombatCharacter *>(obj));
			}
		}
	}
	
	CUtlVector<INextBot *> nextbots;
	TheNextBots().CollectAllBots(&nextbots);
	FOR_EACH_VEC(nextbots, i) {
		INextBot *nextbot = nextbots[i];
		CBaseCombatCharacter *ent = nextbot->GetEntity();
		
		if (!ent->IsPlayer() && ent->IsAlive()) {
			potential_victims.AddToTail(ent);
		}
	}
	
	if (this->m_bDetLostAllHealth) {
		CGameEvent *event = gameeventmanager->CreateEvent("mvm_sentrybuster_killed");
		if (event != nullptr) {
			event->SetInt("sentry_buster", ENTINDEX(actor));
			gameeventmanager->FireEvent(event);
		}
	}
	
	actor->SetMission(CTFBot::MissionType::NONE, false);
	
	actor->m_takedamage = DAMAGE_YES;
	
	FOR_EACH_VEC(potential_victims, i) {
		CBaseCombatCharacter *victim = potential_victims[i];
		
		Vector delta_wsc = victim->WorldSpaceCenter() - actor->WorldSpaceCenter();
		float dist_sqr = delta_wsc.LengthSqr();
		
		if (dist_sqr > (tf_bot_suicide_bomb_range.GetFloat() *
			tf_bot_suicide_bomb_range.GetFloat())) {
			continue;
		}
		
		if (victim->IsPlayer()) {
			UTIL_ScreenFade(victim, COLOR32_WHITE, 1.0f, 0.1f, FFADE_IN);
		}
		
		if (actor->IsLineOfFireClear(victim)) {
			float health = victim->GetHealth();
			if (victim->GetHealth() < victim->GetMaxHealth()) {
				health = victim->GetMaxHealth();
			}
			
			/* NOTE: CTFPlayer::OnTakeDamage reduces the damage to 600 if:
			 * victim is a bot
			 * m_bForceFriendlyFire is true
			 * victim in same team as attacker
			 * victim->IsMiniBoss
			 */
			
			CTakeDamageInfo dmginfo(actor, actor, 4 * health, DMG_BLAST);
			if (tf_bot_suicide_bomb_friendly_fire.GetBool()) {
				dmginfo.SetForceFriendlyFire(true);
			}
			
			CalculateMeleeDamageForce(&dmginfo, VectorNormalize(delta_wsc),
				actor->WorldSpaceCenter());
			victim->TakeDamage(dmginfo);
		}
	}
	
	if (actor->IsAlive()) {
		actor->ForceChangeTeam(TEAM_SPECTATOR, false);
	}
	
	if (this->m_bDetLostAllHealth) {
		CWave *wave = g_pPopulationManager->GetCurrentWave();
		if (wave != nullptr) {
			++wave->m_iSentryBustersKilled;
		}
	}
}
