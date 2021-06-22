/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_destroy_enemy_sentry.cpp
 * used in MvM: TODO
 * 
 * ChangeTo from CTFBotDestroyEnemySentry::Update
 * SuspendFor from CTFBotTacticalMonitor::Update
 * SuspendFor from CTFBotEscort::Update
 * SuspendFor from CTFBotMissionDestroySentries::Update
 */


ConVar tf_bot_debug_destroy_enemy_sentry("tf_bot_debug_destroy_enemy_sentry", "0", FCVAR_CHEAT);
ConVar tf_bot_max_grenade_launch_at_sentry_range("tf_bot_max_grenade_launch_at_sentry_range", "1500", FCVAR_CHEAT);
ConVar tf_bot_max_sticky_launch_at_sentry_range("tf_bot_max_sticky_launch_at_sentry_range", "1500", FCVAR_CHEAT);


CTFBotDestroyEnemySentry::CTFBotDestroyEnemySentry()
{
}

CTFBotDestroyEnemySentry::~CTFBotDestroyEnemySentry()
{
}


const char *CTFBotDestroyEnemySentry::GetName() const
{
	return "DestroyEnemySentry";
}


ActionResult<CTFBot> CTFBotDestroyEnemySentry::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
	this->m_PathFollower.Invalidate();
	
	// 4825 = false
	
	this->m_ctRecomputePath.Invalidate();
	
	// 4826 = false
	
	if (actor->IsPlayerClass(TF_CLASS_DEMOMAN)) {
		this->ComputeCornerAttackSpot(actor);
	} else {
		this->ComputeSafeAttackSpot(actor);
	}
	
	this->m_hSentry = actor->m_hTargetSentry;
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotDestroyEnemySentry::Update(CTFBot *actor, float dt)
{
	if (actor->m_hTargetSentry == nullptr) {
		return ActionResult<CTFBot>::Done("Enemy sentry is destroyed");
	}
	
	if (this->m_hSentry != actor->m_hTargetSentry) {
		return ActionResult<CTFBot>::ChangeTo(new CTFBotDestroyEnemySentry(),
			"Changed sentry target");
	}
	
	
	
	// TODO
}

ActionResult<CTFBot> CTFBotDestroyEnemySentry::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Invalidate();
	this->m_ctRecomputePath.Invalidate();
	
	if (actor->IsPlayerClass(TF_CLASS_DEMOMAN)) {
		this->ComputeCornerAttackSpot(actor);
	} else {
		this->ComputeSafeAttackSpot(actor);
	}
	
	return ActionResult<CTFBot>::Continue();
}


QueryResponse CTFBotDestroyEnemySentry::ShouldHurry(const INextBot *nextbot) const
{
	if (/* 4825 */) {
		return QueryResponse::YES;
	}
	
	return QueryResponse::DONTCARE;
}

QueryResponse CTFBotDestroyEnemySentry::ShouldRetreat(const INextBot *nextbot) const
{
	return QueryResponse::NO;
}

QueryResponse CTFBotDestroyEnemySentry::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	if (/* 4825 */) {
		return QueryResponse::NO;
	}
	
	return QueryResponse::DONTCARE;
}


bool CTFBotDestroyEnemySentry::IsPossible(CTFBot *actor)
{
	if (actor->IsPlayerClass(TF_CLASS_HEAVYWEAPONS) ||
		actor->IsPlayerClass(TF_CLASS_SNIPER) ||
		actor->IsPlayerClass(TF_CLASS_MEDIC) ||
		actor->IsPlayerClass(TF_CLASS_ENGINEER) ||
		actor->IsPlayerClass(TF_CLASS_PYRO) ||
		actor->GetAmmoCount(TF_AMMO_PRIMARY) <= 0 || actor->GetAmmoCount(TF_AMMO_SECONDARY) <= 0 ||
		actor->IsPlayerClass(TF_CLASS_SPY)) {
		return false;
	}
	
	if (TFGameRules()->IsMannVsMachineMode()) {
		return (actor->GetTeamNumber() != TF_TEAM_BLUE);
	}
	
	return true;
}


void CTFBotDestroyEnemySentry::ComputeCornerAttackSpot(CTFBot *actor)
{
	// 4824 = false
	this->m_vecAttackSpot = vec3_origin;
	
	// TODO
}

void CTFBotDestroyEnemySentry::ComputeSafeAttackSpot(CTFBot *actor)
{
	// 4824 = false
	
	// TODO
}


CTFBotUberAttackEnemySentry::CTFBotUberAttackEnemySentry(CObjectSentrygun *sentry) :
	m_hSentry(sentry)
{
}

CTFBotUberAttackEnemySentry::~CTFBotUberAttackEnemySentry()
{
}


const char *CTFBotUberAttackEnemySentry::GetName() const
{
	return "UberAttackEnemySentry";
}


ActionResult<CTFBot> CTFBotUberAttackEnemySentry::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_bSavedIgnoreEnemies = (actor->m_nBotAttrs & CTFBot::AttributeType::IGNOREENEMIES) != 0;
	actor->m_nBotAttrs |= CTFBot::AttributeType::IGNOREENEMIES;
	
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotUberAttackEnemySentry::Update(CTFBot *actor, float dt)
{
	if (!actor->m_Shared.InCond(TF_COND_INVULNERABLE)) {
		return ActionResult<CTFBot>::Done("No longer uber");
	}
	
	if (this->m_hSentry == nullptr) {
		return ActionResult<CTFBot>::Done("Target sentry destroyed");
	}
	
	if (actor->IsPlayerClass(TF_CLASS_DEMOMAN)) {
		// TODO
	}
	
	// TODO
}

void CTFBotUberAttackEnemySentry::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
	if (!this->m_bSavedIgnoreEnemies) {
		actor->m_nBotAttrs &= ~CTFBot::AttributeType::IGNOREENEMIES;
	}
}


QueryResponse CTFBotUberAttackEnemySentry::ShouldHurry(const INextBot *nextbot) const
{
	return QueryResponse::YES;
}

QueryResponse CTFBotUberAttackEnemySentry::ShouldRetreat(const INextBot *nextbot) const
{
	return QueryResponse::NO;
}

QueryResponse CTFBotUberAttackEnemySentry::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	return QueryResponse::YES;
}


FindSafeSentryApproachAreaScan::FindSafeSentryApproachAreaScan(/* ??? */)
{
	// TODO
}

FindSafeSentryApproachAreaScan::~FindSafeSentryApproachAreaScan()
{
}


bool FindSafeSentryApproachAreaScan::operator()(CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar)
{
	// TODO
}

bool FindSafeSentryApproachAreaScan::ShouldSearch(CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar)
{
	// TODO
}

void FindSafeSentryApproachAreaScan::PostSearch()
{
	if (tf_bot_debug_destroy_enemy_sentry.GetBool()) {
		FOR_EACH_VEC(this->m_Areas, i) {
			this->m_Areas[i]->DrawFilled(0x00, 0xff, 0x00, 0xff, 60.0f, true, 5.0f);
		}
	}
}


bool FindGrenadeAim(CTFBot *bot, CBaseEntity *target, float *pYaw, float *pPitch)
{
	bool success = false;
	
	Vector delta = (target->WorldSpaceCenter() - bot->EyePosition());
	
	if (delta.LengthSqr() <= Square(tf_bot_max_grenade_launch_at_sentry_range.GetFloat())) {
		QAngle dir;
		VectorAngles(delta, dir);
		
		float yaw   = actor->EyeAngles().y;
		float pitch = actor->EyeAngles().x;
		
		for (int i = 10; i != 0; --i) {
			Vector est = actor->EstimateProjectileImpactPosition(pitch, yaw, 900.0f);
			
			if ((target->WorldSpaceCenter() - est).LengthSqr() < Square(75.0f)) {
				NextBotIgnoreActorsTraceFilter filter(target);
				
				trace_t tr;
				UTIL_TraceLine(target->WorldSpaceCenter(), est,
					MASK_SOLID_BRUSHONLY, filter, &tr);
				
				if (tr.fraction >= 1.0f && !tr.allsolid && !tr.startsolid) {
					*pYaw   = yaw;
					*pPitch = pitch;
					
					success = true;
					break;
				}
			}
			
			yaw   = RandomFloat(-30.0f, 30.0f) + dir.y;
			pitch = RandomFloat(-85.0f, 85.0f);
		}
	}
	
	return success;
}

bool FindStickybombAim(CTFBot *bot, CBaseEntity *target, float *pYaw, float *pPitch, float *f3)
{
	// TODO: what is f3?
	
	bool success = false;
	
	Vector delta = (target->WorldSpaceCenter() - bot->EyePosition());
	
	if (delta.LengthSqr() <= Square(tf_bot_max_sticky_launch_at_sentry_range.GetFloat())) {
		QAngle dir;
		VectorAngles(delta, dir);
		
		float yaw   = actor->EyeAngles().y;
		float pitch = actor->EyeAngles().x;
		
		for (int i = 100; i != 0; --i) {
			Vector est = actor->EstimateStickybombProjectileImpactPosition(pitch, yaw, 0.0f);
			
			if ((target->WorldSpaceCenter() - est).LengthSqr() < Square(75.0f)) {
				NextBotIgnoreActorsTraceFilter filter(target);
				
				trace_t tr;
				UTIL_TraceLine(target->WorldSpaceCenter(), est,
					MASK_SOLID_BRUSHONLY, filter, &tr);
				
				if (tr.fraction >= 1.0f && !tr.allsolid && !tr.startsolid && *f3 > 0.0f) {
					success = true;
					
					*f3     = 0.0f;
					*pYaw   = yaw;
					*pPitch = pitch;
					
					/* huh? */
					if (*f3 < 0.01) {
						break;
					}
				}
			}
			
			yaw   = RandomFloat(-30.0f, 30.0f) + dir.y;
			pitch = RandomFloat(-85.0f, 85.0f);
		}
	}
	
	return success;
}
