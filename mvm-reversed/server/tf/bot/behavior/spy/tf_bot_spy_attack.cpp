/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/spy/tf_bot_spy_attack.cpp
 * used in MvM: TODO
 */


ConVar tf_bot_spy_knife_range("tf_bot_spy_knife_range", "300", FCVAR_CHEAT,
	"If threat is closer than this, prefer our knife");
ConVar tf_bot_spy_change_target_range_threshold("tf_bot_spy_change_target_range_threshold", "300", FCVAR_CHEAT);


CTFBotSpyAttack::CTFBotSpyAttack(CTFPlayer *victim)
{
	this->m_hVictim = victim;
}

CTFBotSpyAttack::~CTFBotSpyAttack()
{
}


const char *CTFBotSpyAttack::GetName() const
{
	return "SpyAttack";
}


ActionResult<CTFBot> CTFBotSpyAttack::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_ChasePath.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
	
	// 0x4838 = 0
	
	if (this->m_hVictim != nullptr) {
		actor->GetVisionInterface()->AddKnownEntity(this->m_hVictim);
	}
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSpyAttack::Update(CTFBot *actor, float dt)
{
	const CKnownEntity *victim = actor->GetVisionInterface()->GetKnown(this->m_hVictim);
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
	
	// TODO
	
	CBaseObject *obj = actor->GetNearestKnownSappableTarget();
	if (obj != nullptr && actor->IsEntityBetweenTargetAndSelf(obj, victim->GetEntity())) {
		return ActionResult<CTFBot>::ChangeTo(new CTFBotSpySap(obj,
			"Opportunistically sapping an enemy object between my victim and I"));
	}
	
	if (actor->IsAnyEnemySentryAbleToAttackMe()) {
		// 4838 = 1
		
		// TODO: weapon type enum
		actor->Weapon_Switch(actor->Weapon_GetByType(0));
		
		return ActionResult<CTFBot>::ChangeTo(new CTFBotRetreatToCover(),
			"Escaping sentry fire!");
	}
	
	CTFPlayer *victim_player = ToTFPlayer(victim->GetEntity());
	if (victim_player == nullptr) {
		return ActionResult<CTFBot>::Done("Current 'threat' is not a player or a building?");
	}
	
	
	
	// TODO
}

ActionResult<CTFBot> CTFBotSpyAttack::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_ChasePath.Invalidate();
	
	this->m_hVictim = nullptr;
	
	// 0x4838 = 0
	
	return ActionResult<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotSpyAttack::OnContact(CTFBot *actor, CBaseEntity *ent, CGameTrace *trace)
{
	if (actor->IsEnemy(ent) && ToBaseCombatCharacter(ent) != nullptr) {
		CBaseCombatCharacter *enemy = ToBaseCombatCharacter(ent);
		
		if (enemy->IsLookingTowards(actor, 0.9f)) {
			// 0x4838 = 1
		}
	}
	
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotSpyAttack::OnStuck(CTFBot *actor)
{
	return EventDesiredResult<CTFBot>::Continue();
}

EventDesiredResult<CTFBot> CTFBotSpyAttack::OnInjured(CTFBot *actor, const CTakeDamageInfo& info)
{
	if (actor->IsEnemy(info.GetAttacker()) && !actor->m_Shared.InCond(TF_COND_DISGUISED)) {
		// TODO: weapon switch stuff
		
		return ActionResult<CTFBot>::ChangeTo(new CTFBotRetreatToCover(),
			"Time to get out of here!");
	}
	
	return ActionResult<CTFBot>::Continue();
}


QueryResponse CTFBotSpyAttack::ShouldHurry(const INextBot *nextbot) const
{
	return QueryResponse::YES;
}

QueryResponse CTFBotSpyAttack::ShouldRetreat(const INextBot *nextbot) const
{
	return QueryResponse::DONTCARE;
}

QueryResponse CTFBotSpyAttack::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	CTFBot *actor = ToTFBot(nextbot->GetEntity());
	
	if (!/* 0x4838 */ &&
		!actor->m_Shared.InCond(TF_COND_BURNING) &&
		!actor->m_Shared.InCond(TF_COND_URINE) &&
		!actor->m_Shared.InCond(TF_COND_STEALTHED_BLINK) &&
		!actor->m_Shared.InCond(TF_COND_BLEEDING)) {
		return false;
	}
	
	return true;
}

QueryResponse CTFBotSpyAttack::IsHindrance(const INextBot *nextbot, CBaseEntity *it) const
{
	/* dunno what the purpose of the -1 check is */
	if (ent != (CBaseEntity *)-1 && ent != nullptr && this->m_hVictim != nullptr) {
		if (ENTINDEX(ent) == ENTINDEX(this->m_hVictim)) {
			return QueryResponse::NO;
		}
	}
	
	return QueryResponse::DONTCARE;
}

const CKnownEntity *CTFBotSpyAttack::SelectMoreDangerousThreat(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const
{
	CTFBot *actor = ToTFBot(nextbot->GetEntity());
	
	if (!actor->IsSelf(them)) {
		return nullptr;
	}
	
	CTFWeaponBase *weapon = actor->GetActiveTFWeapon();
	if (weapon == nullptr || !weapon->IsMeleeWeapon()) {
		return nullptr;
	}
	
	float dist1 = actor->GetRangeSquaredTo(threat1->GetEntity());
	float dist2 = actor->GetRangeSquaredTo(threat2->GetEntity());
	
	if (dist1 < dist2) {
		return threat1;
	} else {
		return threat2;
	}
}
