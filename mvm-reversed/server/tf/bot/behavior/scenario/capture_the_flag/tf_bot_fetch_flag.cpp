/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/scenario/capture_the_flag/tf_bot_fetch_flag.cpp
 * used in MvM: TODO
 */


CTFBotFetchFlag::CTFBotFetchFlag(bool give_up_when_done)
{
	this->m_bGiveUpWhenDone = give_up_when_done;
}

CTFBotFetchFlag::~CTFBotFetchFlag()
{
}


const char *CTFBotFetchFlag::GetName() const
{
	return "FetchFlag";
}


ActionResult<CTFBot> CTFBotFetchFlag::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotFetchFlag::Update(CTFBot *actor, float dt)
{
	CCaptureFlag *flag = actor->GetFlagToFetch();
	if (flag == nullptr) {
		if (TFGameRules()->IsMannVsMachineMode()) {
			return ActionResult<CTFBot>::SuspendFor(new CTFBotAttackFlagDefenders(),
				"Flag flag exists - Attacking the enemy flag defenders");
		} else {
			return ActionResult<CTFBot>::Done("No flag");
		}
	}
	
	if (actor->m_Shared.IsStealthed()) {
		actor->PressAltFireButton();
	}
	
	/* auto flag pickup right when MvM bots spawn and the flag is at home */
	if (TFGameRules()->IsMannVsMachineMode() && flag->IsHome()) {
		if (gpGlobals->curtime - /* float @ actor+0x2774 */ < 1.0f &&
			actor->GetTeamNumber() != TEAM_SPECTATOR) {
			flag->PickUp(actor, true);
		} else {
			if (this->m_bGiveUpWhenDone) {
				return ActionResult<CTFBot>::Done("Flag unreachable");
			} else {
				return ActionResult<CTFBot>::SuspendFor(new CTFBotAttackFlagDefenders(),
					"Flag is unreachable at home - Attacking the enemy flag defenders");
			}
		}
	}
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
	if (threat != nullptr && threat->IsVisibleRecently()) {
		actor->EquipBestWeaponForThreat(threat);
	}
	
	CTFPlayer *carrier = ToTFPlayer(flag->GetOwnerEntity());
	if (carrier != nullptr) {
		if (this->m_bGiveUpWhenDone) {
			return ActionResult<CTFBot>::Done("Someone else picked up the flag");
		} else {
			return ActionResult<CTFBot>::SuspendFor(new CTFBotAttackFlagDefenders(),
				"Someone has the flag - attacking the enemy defenders");
		}
	}
	
	if (this->m_ctRecomputePath.IsElapsed()) {
		CTFBotPathCost cost_func(actor, DEFAULT_ROUTE);
		bool success = this->m_PathFollower.Compute(actor, flag->WorldSpaceCenter(), cost_func, 0.0f, true);
		
		if (!success && flag->IsDropped()) {
			return ActionResult<CTFBot>::SuspendFor(new CTFBotAttackFlagDefenders(RandomFloat(5.0f, 10.0f)),
				"Flag unreachable - Attacking");
		}
		
		this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
	}
	
	this->m_PathFollower.Update(actor);
	
	return ActionResult<CTFBot>::Continue();
}


QueryResponse CTFBotFetchFlag::ShouldHurry(const INextBot *nextbot) const
{
	return QueryResponse::YES;
}

QueryResponse CTFBotFetchFlag::ShouldRetreat(const INextBot *nextbot) const
{
	return QueryResponse::NO;
}
