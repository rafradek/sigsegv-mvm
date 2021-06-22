/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/sniper/tf_bot_sniper_lurk.cpp
 * used in MvM: TODO
 */


ConVar tf_bot_sniper_patience_duration("tf_bot_sniper_patience_duration", "10", FCVAR_CHEAT,
	"How long a Sniper bot will wait without seeing an enemy before picking a new spot");
ConVar tf_bot_sniper_target_linger_duration("tf_bot_sniper_target_linger_duration", "2", FCVAR_CHEAT,
	"How long a Sniper bot will keep toward at a target it just lost sight of");
ConVar tf_bot_sniper_allow_opportunistic("tf_bot_sniper_allow_opportunistic", "1", FCVAR_NONE,
	"If set, Snipers will stop on their way to their preferred lurking spot to snipe at opportunistic targets");
ConVar tf_mvm_bot_sniper_target_by_dps("tf_mvm_bot_sniper_target_by_dps", "1", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY,
	"If set, Snipers in MvM mode target the victim that has the highest DPS");


CTFBotSniperLurk::CTFBotSniperLurk()
{
	// TODO
}

CTFBotSniperLurk::~CTFBotSniperLurk()
{
}


const char *CTFBotSniperLurk::GetName() const
{
	return "SniperLurk";
}


ActionResult<CTFBot> CTFBotSniperLurk::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_ctPatience.Start(RandomFloat(0.9f, 1.1f) * tf_bot_sniper_patience_duration.GetFloat());
	
	this->m_vecHome = actor->GetAbsOrigin();
	this->m_bHasHome = false;
	this->m_bNearHome = false;
	// TODO: dword @ 0x4820 = 0
	this->m_bOpportunistic = tf_bot_sniper_allow_opportunistic.GetBool();
	
	CBaseEntity *ent = nullptr;
	while ((ent = gEntList.FindEntityByClassname(ent, "func_tfbot_hint")) != nullptr) {
		auto hint = static_cast<CTFBotHint *>(ent);
		
		// TODO: is m_hint an enum or something?
		if (hint->m_hint == 0) {
			this->m_Hints.AddToTail(hint);
			
			if (actor->IsSelf(hint->GetOwnerEntity())) {
				hint->SetOwnerEntity(nullptr);
			}
		}
	}
	
	this->m_hHint = nullptr;
	
	if (TFGameRules()->IsMannVsMachineMode() && actor->GetTeamNumber() == TF_TEAM_BLUE) {
		actor->SetMission(CTFBot::MissionType::SNIPER, false);
	}
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSniperLurk::Update(CTFBot *actor, float dt)
{
	actor->AccumulateSniperSpots();
	
	if (!this->m_bHasHome) {
		this->FindNewHome(actor);
	}
	
	bool attacking = false;
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	if (threat != nullptr && threat->GetEntity()->IsAlive() &&
		actor->GetIntentionInterface()->ShouldAttack(actor, threat)) {
		if (threat->IsVisibleInFOVNow()) {
			// dword @ 0x4820 = 0
			
			float dist_sqr = threat->GetLastKnownPosition().DistToSqr(actor->GetAbsOrigin());
			if (dist_sqr >= Square(tf_bot_sniper_melee_range.GetFloat())) {
				return ActionResult<CTFBot>::SuspendFor(new CTFBotMeleeAttack(1.25f * tf_bot_sniper_melee_range.GetFloat()),
					"Melee attacking nearby threat");
			}
		}
		
		if (threat->GetTimeSinceLastSeen() < tf_bot_sniper_target_linger_duration.GetFloat() &&
			actor->IsLineOfFireClear(threat->GetEntity())) {
			if (this->m_bOpportunistic) {
				CBaseCombatWeapon *primary = actor->Weapon_GetSlot(0);
				if (primary != nullptr) {
					actor->Weapon_Switch(primary);
				}
				
				this->m_ctPatience.Reset();
				
				attacking = true;
				
				if (!this->m_bHasHome) {
					this->m_vecHome = actor->GetAbsOrigin();
					
					this->m_ctPatience.Start(RandomFloat(0.9f, 1.1f) * tf_bot_sniper_patience_duration.GetFloat());
					
					attacking = true;
				}
			} else {
				attacking = false;
				
				CBaseCombatWeapon *secondary = actor->Weapon_GetSlot(1);
				if (secondary != nullptr) {
					actor->Weapon_Switch(secondary);
				}
			}
		}
	}
	
	float dsqr_from_home = (actor->GetAbsOrigin().AsVector2D() - this->m_vecHome).LengthSqr();
	this->m_bNearHome = (dsqr_from_home < Square(25.0f));
	
	if (dsqr_from_home < Square(25.0f)) {
		this->m_bOpportunistic = tf_bot_sniper_allow_opportunistic.GetBool();
		
		if (this->m_ctPatience.IsElapsed()) {
			// increment int @ 0x4820
			
			if (this->FindNewHome(actor)) {
				actor->SpeakConceptIfAllowed(MP_CONCEPT_PLAYER_NEGATIVE);
				
				this->m_ctPatience.Start(RandomFloat(0.9f, 1.1f) * tf_bot_sniper_patience_duration.GetFloat());
			} else {
				this->m_ctPatience.Start(1.0f);
			}
		}
	} else {
		this->m_ctPatience.Reset();
		
		if (!attacking) {
			if (this->m_ctRecomputePath.IsElapsed()) {
				this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
				
				CTFBotPathCost cost_func(actor, SAFEST_ROUTE);
				this->m_PathFollower.Compute(actor, this->m_vecHome, cost_func, 0.0f, true);
			}
			
			this->m_PathFollower.Update(actor);
			
			if (actor->m_Shared.InCond(TF_COND_ZOOMED)) {
				actor->PressAltFireButton();
			}
			
			return ActionResult<CTFBot>::Continue();
		}
	}
	
	CBaseCombatWeapon *primary = actor->Weapon_GetSlot(0);
	if (primary != nullptr) {
		actor->Weapon_Switch(primary);
	
		auto weapon = static_cast<CTFWeaponBase *>(primary);
		if (!actor->m_Shared.InCond(TF_COND_ZOOMED) && !weapon->IsWeapon(TF_WEAPON_COMPOUND_BOW)) {
			actor->PressAltFireButton();
		}
	}
	
	return ActionResult<CTFBot>::Continue();
}

void CTFBotSniperLurk::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
	if (actor->m_Shared.InCond(TF_COND_ZOOMED)) {
		actor->PressAltFireButton();
	}
	
	if (this->m_hHint != nullptr) {
		this->m_hHint->SetOwnerEntity(nullptr);
		
		if (tf_bot_debug_sniper.GetBool()) {
			DevMsg("%3.2f: %s: Releasing hint.\n", gpGlobals->curtime, actor->GetPlayerName());
		}
	}
}

ActionResult<CTFBot> CTFBotSniperLurk::OnSuspend(CTFBot *actor, Action<CTFBot> *action)
{
	if (actor->m_Shared.InCond(TF_COND_ZOOMED)) {
		actor->PressAltFireButton();
	}
	
	if (this->m_hHint != nullptr) {
		this->m_hHint->SetOwnerEntity(nullptr);
		
		if (tf_bot_debug_sniper.GetBool()) {
			DevMsg("%3.2f: %s: Releasing hint.\n", gpGlobals->curtime, actor->GetPlayerName());
		}
	}
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSniperLurk::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_ctRecomputePath.Invalidate();
	
	this->m_hHint = nullptr;
	this->FindNewHome(actor);
	
	return ActionResult<CTFBot>::Continue();
}


QueryResponse CTFBotSniperLurk::ShouldRetreat(const INextBot *nextbot) const
{
	if (TFGameRules()->IsMannVsMachineMode() &&
		nextbot->GetEntity()->GetTeamNumber() == TF_TEAM_BLUE) {
		return QueryResponse::NO;
	}
	
	return QueryResponse::DONTCARE;
}

QueryResponse CTFBotSniperLurk::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	CTFNavArea *area = static_cast<CTFNavArea *>(actor->GetLastKnownArea());
	
	if (TFGameRules()->IsMannVsMachineMode() && area != nullptr &&
		(area->m_nAttributes & BLUE_SPAWN_ROOM) != 0) {
		return QueryResponse::NO;
	}
	
	return QueryResponse::YES;
}

const CKnownEntity *CTFBotSniperLurk::SelectMoreDangerousThreat(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const
{
	if (TFGameRules()->IsMannVsMachineMode() && tf_mvm_bot_sniper_target_by_dps.GetBool()) {
		CTFBot *actor = ToTFBot(nextbot->GetEntity());
		
		if (!threat1->IsVisibleRecently() && threat2->IsVisibleRecently()) {
			return threat2;
		}
		if (threat1->IsVisibleRecently() && !threat2->IsVisibleRecently()) {
			return threat1;
		}
		
		CTFPlayer *player1 = ToTFPlayer(threat1->GetEntity());
		CTFPlayer *player2 = ToTFPlayer(threat2->GetEntity());
		if (player1 == nullptr || player2 == nullptr) {
			return nullptr;
		}
		
		float dsqr1 = actor->GetRangeSquaredTo(player1);
		float dsqr2 = actor->GetRangeSquaredTo(player2);
		
		if ((actor->m_nRestrict & CTFBot::WeaponRestriction::MELEEONLY) == 0) {
			if (dsqr1 >= Square(500.0f) && dsqr2 < Square(500.0f)) {
				return threat2;
			}
			if (dsqr1 < Square(500.0f) && dsqr2 >= Square(500.0f)) {
				return threat1;
			}
			
			// TODO: CTFPlayer+0x2930
			// (prefer threat with higher value, presumably this is a DPS figure)
		}
		
		if (dsqr2 > dsqr1) {
			return threat1;
		}
		return threat2;
	}
	
	return nullptr;
}


CTFBotHint *CTFBotSniperLurk::FindHint(CTFBot *actor)
{
	// TODO
}

bool CTFBotSniperLurk::FindNewHome(CTFBot *actor)
{
	// TODO
}
