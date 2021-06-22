/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/demoman/tf_bot_stickybomb_sentrygun.cpp
 * used in MvM: TODO
 */


ConVar tf_bot_sticky_base_range("tf_bot_sticky_base_range", "800", FCVAR_CHEAT);
ConVar tf_bot_sticky_charge_rate("tf_bot_sticky_charge_rate", "0.01", FCVAR_CHEAT,
	"Seconds of charge per unit range beyond base");


CTFBotStickybombSentrygun::CTFBotStickybombSentrygun(CObjectSentrygun *sentry)
{
	if (sentry != nullptr) {
		this->m_hSentry = sentry;
	}
	
	this->m_bOpportunistic = false;
}

CTFBotStickybombSentrygun::CTFBotStickybombSentrygun(CObjectSentrygun *sentry, float x, float y, float z)
{
	if (sentry != nullptr) {
		this->m_hSentry = sentry;
	}
	
	this->m_bOpportunistic = true;
	
	// 0034 = (x, y, z)
}

CTFBotStickybombSentrygun::~CTFBotStickybombSentrygun()
{
}


const char *CTFBotStickybombSentrygun::GetName() const
{
	return "StickybombSentrygun";
}


ActionResult<CTFBot> CTFBotStickybombSentrygun::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	actor->PressAltFireButton();
	
	actor->m_bLookAroundForEnemies = false;
	
	this->m_bReload = true;
	
	actor->SetAbsVelocity(vec3_origin);
	
	this->m_ctAimTimeout.Start(3.0f);
	
	if (this->m_bOpportunistic) {
		// 0058 = true
		
		// 0068 = actor->GetAbsOrigin();
		
		// 0048 = true
		
		// 0074 = 0034 .z
		
		// TODO: AngleVectors, 1500, EyePosition
		
		// 005c = ...
	}
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotStickybombSentrygun::Update(CTFBot *actor, float dt)
{
	CTFWeaponBase *active = actor->m_Shared.GetActiveTFWeapon();
	auto launcher = dynamic_cast<CTFPipebombLauncher *>(actor->Weapon_GetSlot(1));
	if (launcher == nullptr || active == nullptr) {
		return ActionResult<CTFBot>::Done("Missing weapon");
	}
	
	if (active->GetWeaponID() != TF_WEAPON_PIPEBOMBLAUNCHER) {
		actor->Weapon_Switch(launcher);
	}
	
	if (this->m_hSentry == nullptr || !this->m_hSentry->IsAlive()) {
		return ActionResult<CTFBot>::Done("Sentry destroyed");
	}
	
	if (!/* 0058 */ && this->m_ctAimTimeout.IsElapsed()) {
		return ActionResult<CTFBot>::Done("Can't find aim");
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
		
		return ActionResult<CTFBot>::Continue();
	}
	
	int wanted_stickies;
	if (TFGameRules()->IsMannVsMachineMode()) {
		wanted_stickies = 5;
	} else {
		wanted_stickies = 3;
	}
	
	if (launcher->GetPipeBombCount() >= wanted_stickies || actor->GetAmmoCount(TF_AMMO_SECONDARY) <= 0) {
		// TODO
		// stuff related to launcher->m_Pipebombs
		// pipebomb+0x4f9: m_bTouched
		
		actor->PressAltFireButton();
		
		if (actor->GetAmmoCount(TF_AMMO_SECONDARY) <= 0) {
			return ActionResult<CTFBot>::Done("Out of ammo");
		}
		
		return ActionResult<CTFBot>::Continue();
	}
	
	if (/* 0048 */) {
		float charge_time = 4.4f * this->m_flChargeLevel;
		
		actor->GetBodyInterface()->AimHeadTowards(this->m_vecAimTarget,
			IBody::LookAtPriorityType::CRITICAL, 0.3f, nullptr, "Aiming a sticky bomb at a sentrygun");
		
		if (gpGlobals->curtime - launcher->GetChargeBeginTime() < charge_time) {
			actor->PressFireButton();
		} else {
			actor->ReleaseFireButton();
			// 0048 = false
		}
		
		return ActionResult<CTFBot>::Continue();
	}
	
	if (gpGlobals->curtime <= launcher->m_flNextPrimaryAttack) {
		return ActionResult<CTFBot>::Continue();
	}
	
	if (/* 0058 */) {
		if (actor->IsRangeGreaterThan(/* 0068 */, 1.0f)) {
			// 0058 = false
			this->m_ctAimTimeout.Reset();
		}
		
		if (/* 0058 */) {
			// TODO: goto LABEL_56
		}
	}
	
	// TODO: EyePosition and onward
	// ...
	// TODO
	
	if (/* 0058 */) {
		// TODO
	}
	
	return ActionResult<CTFBot>::Continue();
}

void CTFBotStickybombSentrygun::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
	actor->PressAltFireButton();
	
	actor->m_bLookAroundForEnemies = true;
}

ActionResult<CTFBot> CTFBotStickybombSentrygun::OnSuspend(CTFBot *actor, Action<CTFBot> *action)
{
	actor->PressAltFireButton();
	
	return ActionResult<CTFBot>::Done();
}


EventDesiredResult<CTFBot> CTFBotStickybombSentrygun::OnInjured(CTFBot *actor, const CTakeDamageInfo& info)
{
	return EventDesiredResult<CTFBot>::Done("Ouch!", ResultSeverity::MEDIUM);
}


QueryResponse CTFBotStickybombSentrygun::ShouldHurry(const INextBot *nextbot) const
{
	return QueryResponse::YES;
}

QueryResponse CTFBotStickybombSentrygun::ShouldRetreat(const INextBot *nextbot) const
{
	return QueryResponse::NO;
}

QueryResponse CTFBotStickybombSentrygun::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	return QueryResponse::NO;
}


bool CTFBotStickybombSentrygun::IsAimOnTarget(CTFBot *actor, float pitch, float yaw, float speed)
{
	Vector est = actor->EstimateStickybombProjectileImpactPosition(pitch, yaw, speed);
	
	if ((this->m_hSentry->WorldSpaceCenter() - est).LengthSqr() < Square(75.0f)) {
		trace_t tr;
		UTIL_TraceLine(this->m_hSentry->WorldSpaceCenter(), est,
			MASK_SOLID_BRUSHONLY, nullptr, 0, &tr);
		
		if (tr.fraction >= 1.0f && !tr.allsolid && !tr.startsolid) {
			return true;
		}
	}
	
	return false;
}
