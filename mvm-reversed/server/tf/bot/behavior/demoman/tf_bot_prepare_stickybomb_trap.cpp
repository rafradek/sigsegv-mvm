/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/demoman/tf_bot_prepare_stickybomb_trap.cpp
 * used in MvM: TODO
 */


ConVar tf_bot_stickybomb_density("tf_bot_stickybomb_density", 0.0001, FCVAR_CHEAT,
	"Number of stickies to place per square inch");


/* BUG: use of a static variable for the aim reply means that multiple bots
 * can't be aiming stickies at the same time */
PlaceStickyBombReply bombReply;


CTFBotPrepareStickybombTrap::CTFBotPrepareStickybombTrap()
{
	this->m_LastKnownArea = nullptr;
}

CTFBotPrepareStickybombTrap::~CTFBotPrepareStickybombTrap()
{
	bombReply.Reset();
}


const char *CTFBotPrepareStickybombTrap::GetName() const
{
	return "PrepareStickybombTrap";
}


ActionResult<CTFBot> CTFBotPrepareStickybombTrap::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	auto launcher = dynamic_cast<CTFPipebombLauncher *>(actor->Weapon_GetSlot(1));
	if (launcher != nullptr &&
		actor->GetAmmoCount(TF_AMMO_SECONDARY) >= launcher->GetMaxClip1() &&
		launcher->Clip1() < launcher->GetMaxClip1()) {
		this->m_bReload = true;
	} else {
		this->m_bReload = false;
	}
	
	this->m_LastKnownArea = actor->GetLastKnownArea();
	if (this->m_LastKnownArea == nullptr) {
		return ActionResult<CTFBot>::Done("No nav mesh");
	}
	
	this->InitBombTargetAreas(actor);
	
	actor->m_bLookAroundForEnemies = false;
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotPrepareStickybombTrap::Update(CTFBot *actor, float dt)
{
	if (!TFGameRules()->InSetup()) {
		const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
		if (threat != nullptr && (threat->GetLastKnownPosition() - actor->GetAbsOrigin()).LengthSqr() < Square(500.0f))) {
			return ActionResult<CTFBot>::Done("Enemy nearby - giving up");
		}
	}
	
	if (actor->GetLastKnownArea() != nullptr && actor->GetLastKnownArea() != this->m_LastKnownArea) {
		this->m_LastKnownArea = actor->GetLastKnownArea();
		this->InitBombTargetAreas(actor);
	}
	
	CTFWeaponBase *active = actor->m_Shared.GetActiveTFWeapon();
	auto launcher = dynamic_cast<CTFPipebombLauncher *>(actor->Weapon_GetSlot(1));
	
	if (launcher == nullptr || active == nullptr) {
		return ActionResult<CTFBot>::Done("Missing weapon");
	}
	
	if (active->GetWeaponID() != TF_WEAPON_PIPEBOMBLAUNCHER) {
		actor->Weapon_Switch(launcher);
	}
	
	if (this->m_bReload) {
		int clip_size;
		if (actor->GetAmmoCount(TF_AMMO_SECONDARY) < launcher->GetMaxClip1()) {
			clip_size = actor->GetAmmoCount(TF_AMMO_SECONDARY);
		} else {
			clip_size = launcher->GetMaxClip1();
		}
		
		if (launcher->Clip1() >= clip_size) {
			this->m_bReload = false;
		}
		
		actor->PressReloadButton();
	} else {
		if (launcher->GetPipeBombCount() >= 8 || actor->GetAmmoCount(TF_AMMO_SECONDARY) <= 0) {
			return ActionResult<CTFBot>::Done("Max sticky bombs reached");
		} else {
			if (this->m_ctAimTimeout.IsElapsed()) {
				FOR_EACH_VEC(this->m_BombTargetAreas, i) {
					BombTargetArea *target = this->m_BombTargetAreas[i];
					
					int wanted_stickies = Max(1, (int)(target->area->GetSizeX() * target->area->GetSizeY() *
						tf_bot_stickybomb_density.GetFloat()));
					
					if (target->stickies < wanted_stickies) {
						bombReply.Init(target, &this->m_ctAimTimeout);
						this->m_ctAimTimeout.Start(2.0f);
						
						actor->GetBodyInterface->AimHeadTowards(target->area->GetRandomPoint(),
							IBody::LookAtPriorityType::IMPORTANT, 5.0f, &bombReply, "Aiming a sticky bomb");
						
						return ActionResult<CTFBot>::Continue();
					}
				}
				
				return ActionResult<CTFBot>::Done("Exhausted bomb target areas");
			}
		}
	}
	
	return ActionResult<CTFBot>::Continue();
}

void CTFBotPrepareStickybombTrap::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
	actor->GetBodyInterface()->ClearPendingAimReply();
	
	actor->m_bLookAroundForEnemies = true;
}

ActionResult<CTFBot> CTFBotPrepareStickybombTrap::OnSuspend(CTFBot *actor, Action<CTFBot> *action)
{
	return ActionResult<CTFBot>::Done();
}


EventDesiredResult<CTFBot> CTFBotPrepareStickybombTrap::OnInjured(CTFBot *actor, const CTakeDamageInfo& info)
{
	return EventDesiredResult<CTFBot>::Done("Ouch!", ResultSeverity::MEDIUM);
}


QueryResponse CTFBotPrepareStickybombTrap::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	return QueryResponse::NO;
}


static bool CTFBotPrepareStickybombTrap::IsPossible(CTFBot *actor)
{
	if (actor->GetTimeSinceLastInjury() < 1.0f) {
		return false;
	}
	
	if (!actor->IsPlayerClass(TF_CLASS_DEMOMAN)) {
		return false;
	}
	
	auto launcher = dynamic_cast<CTFPipebombLauncher *>(actor->Weapon_GetSlot(1));
	if (launcher != nullptr && !actor->IsWeaponRestricted(launcher)) {
		if (launcher->GetPipeBombCount() >= 8) {
			return false;
		}
		
		if (actor->GetAmmoCount(TF_AMMO_SECONDARY) <= 0) {
			return false;
		}
	}
	
	return true;
}


void CTFBotPrepareStickybombTrap::InitBombTargetAreas(CTFBot *actor)
{
	/* intentional array copy */
	CUtlVector<CTFNavArea *> areas = *TheNavMesh->GetInvasionAreas(actor->GetTeamNumber());
	areas.Shuffle();
	
	this->m_BombTargetAreas.RemoveAll();
	
	FOR_EACH_VEC(areas, i) {
		BombTargetArea target;
		target.area = areas[i];
		target.stickies = 0;
		
		this->m_BombTargetAreas.AddToTail(target);
	}
	
	this->m_ctAimTimeout.Invalidate();
	
	actor->GetBodyInterface()->ClearPendingAimReply();
}


void PlaceStickyBombReply::OnSuccess(INextBot *nextbot)
{
	CTFBot *actor = ToTFBot(nextbot->GetEntity());
	
	CTFWeaponBase *weapon = actor->GetActiveTFWeapon();
	if (weapon != nullptr && weapon->GetWeaponID() == TF_WEAPON_PIPEBOMBLAUNCHER) {
		actor->PressFireButton(0.1f);
		
		if (this->m_pBombTargetArea != nullptr) {
			++this->m_pBombTargetArea->stickies;
		}
		
		if (this->m_pctAimTimeout != nullptr) {
			this->m_pctAimTimeout.Start(0.15f);
		}
	}
}

void PlaceStickyBombReply::OnFail(INextBot *nextbot, FailureReason reason)
{
	/* BUG: doesn't check for nullptr */
	this->m_pctAimTimeout->Invalidate();
}


void PlaceStickyBombReply::Init(BombTargetArea *target, CountdownTimer *timer)
{
	this->m_pBombTargetArea = target;
	this->m_pctAimTimeout   = timer;
}

void PlaceStickyBombReply::Reset()
{
	this->m_pBombTargetArea = nullptr;
	this->m_pctAimTimeout   = nullptr;
}
