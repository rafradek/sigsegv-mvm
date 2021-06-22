/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_melee_attack.cpp
 * used in MvM: TODO
 * 
 * member of CTFBotEscortSquadLeader
 * member of CTFBotEscortFlagCarrier
 * SuspendFor from CTFBotSniperLurk::Update
 */


ConVar tf_bot_melee_attack_abandon_range("tf_bot_melee_attack_abandon_range", "500", FCVAR_CHEAT,
	"If threat is farther away than this, bot will switch back to its primary weapon and attack");


CTFBotMeleeAttack::CTFBotMeleeAttack(float abandon_range = -1.0f)
{
	if (abandon_range < 0.0f) {
		this->m_flAbandonRange = tf_bot_melee_attack_abandon_range.GetFloat();
	} else {
		this->m_flAbandonRange = abandon_range;
	}
}

CTFBotMeleeAttack::~CTFBotMeleeAttack()
{
}


const char *CTFBotMeleeAttack::GetName() const
{
	return "MeleeAttack";
}


ActionResult<CTFBot> CTFBotMeleeAttack::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_ChasePath.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotMeleeAttack::Update(CTFBot *actor, float dt)
{
	const CKnownEntity *threat = actor->GetPrimaryKnownThreat(false);
	if (threat == nullptr) {
		return ActionResult<CTFBot>::Done("No threat");
	}
	
	if ((threat->GetLastKnownPosition() - actor->GetAbsOrigin()).LengthSqr() > Square(this->m_flAbandonRange)) {
		return ActionResult<CTFBot>::Done("Threat is too far away for a melee attack");
	}
	
	CBaseCombatWeapon *melee = actor->Weapon_GetSlot(2);
	if (melee != nullptr) {
		actor->Weapon_Switch(melee);
	}
	
	actor->PressFireButton();
	
	CTFBotCostFunc cost_func(actor, FASTEST_ROUTE);
	this->m_ChasePath.Update(actor, threat->GetEntity(), cost_func, nullptr);
	
	return ActionResult<CTFBot>::Continue();
}
