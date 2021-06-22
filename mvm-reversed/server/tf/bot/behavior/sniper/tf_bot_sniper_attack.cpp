/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/sniper/tf_bot_sniper_attack.cpp
 * used in MvM: TODO
 */


ConVar tf_bot_sniper_flee_range("tf_bot_sniper_flee_range", "400", FCVAR_CHEAT,
	"If threat is closer than this, retreat");
ConVar tf_bot_sniper_melee_range("tf_bot_sniper_melee_range", "200", FCVAR_CHEAT,
	"If threat is closer than this, attack with melee weapon");
ConVar tf_bot_sniper_linger_time("tf_bot_sniper_linger_time", "5", FCVAR_CHEAT,
	"How long Sniper will wait around after losing his target before giving up");


CTFBotSniperAttack::CTFBotSniperAttack(/* TODO */)
{
	// TODO
}

CTFBotSniperAttack::~CTFBotSniperAttack()
{
}


const char *CTFBotSniperAttack::GetName() const
{
	return "SniperAttack";
}


ActionResult<CTFBot> CTFBotSniperAttack::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSniperAttack::Update(CTFBot *actor, float dt)
{
	CBaseCombatWeapon *primary = actor->Weapon_GetSlot(0);
	if (primary != nullptr) {
		actor->Weapon_Switch(primary);
	}
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
	if (threat == nullptr || !threat->GetEntity()->IsAlive() || !threat->IsVisibleInFOVNow()) {
		if (this->m_ctLinger.IsElapsed() && !actor->m_Shared.InCond(TF_COND_ZOOMED)) {
			return ActionResult<CTFBot>::Done("No threat for awhile");
		}
		
		return ActionResult<CTFBot>::Continue();
	}
	
	actor->EquipBestWeaponForThreat(threat);
	
	if ((actor->GetAbsOrigin() - threat->GetLastKnownPosition()).LengthSqr() < Square(tf_bot_sniper_flee_range.GetFloat())) {
		return ActionResult<CTFBot>::SuspendFor(new CTFBotRetreatToCover(),
			"Retreating from nearby enemy");
	}
	
	if (actor->GetTimeSinceLastInjury() < 1.0f) {
		return ActionResult<CTFBot>::SuspendFor(new CTFBotRetreatToCover(),
			"Retreating due to injury");
	}
	
	this->m_ctLinger.Start(RandomFloat(0.75f, 1.25f) * tf_bot_sniper_linger_time.GetFloat());
	
	if (!actor->m_Shared.InCond(TF_COND_ZOOMED)) {
		actor->PressAltFireButton();
	}
	
	return ActionResult<CTFBot>::Continue();
}

void CTFBotSniperAttack::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
	if (actor->m_Shared.InCond(TF_COND_ZOOMED)) {
		actor->PressAltFireButton();
	}
}

ActionResult<CTFBot> CTFBotSniperAttack::OnSuspend(CTFBot *actor, Action<CTFBot> *action)
{
	if (actor->m_Shared.InCond(TF_COND_ZOOMED)) {
		actor->PressAltFireButton();
	}
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotSniperAttack::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	return ActionResult<CTFBot>::Continue();
}


Vector CTFBotSniperAttack::SelectTargetPoint(const INextBot *nextbot, const CBaseCombatCharacter *them) const
{
	VPROF_BUDGET("CTFBotSniperAttack::SelectTargetPoint", "NextBot");
	
	// TODO
}

const CKnownEntity *CTFBotSniperAttack::SelectMoreDangerousThreat(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const
{
	// TODO
}


bool CTFBotSniperAttack::IsPossible(CTFBot *actor)
{
	if (actor->IsPlayerClass(TF_CLASS_SNIPER) && actor->GetVisionInterface()->GetPrimaryKnownThreat(false) != nullptr &&
		actor->GetVisionInterface()->GetPrimaryKnownThreat(false)->IsVisibleRecently()) {
		return true;
	}
	
	return false;
}


bool CTFBotSniperAttack::IsImmediateThreat(const CBaseCombatCharacter *who, const CKnownEntity *threat) const
{
	// TODO
}
