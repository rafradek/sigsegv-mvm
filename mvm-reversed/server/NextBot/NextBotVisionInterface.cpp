/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/NextBotVisionInterface.cpp
 */


ConVar nb_blind("nb_blind", "0", FCVAR_CHEAT,
	"Disable vision");
ConVar nb_debug_known_entities("nb_debug_known_entities", "0", FCVAR_CHEAT,
	"Show the 'known entities' for the bot that is the current spectator target");


IVision::IVision(INextBot *nextbot)
	: INextBotComponent(nextbot)
{
	this->Reset();
}

IVision::~IVision()
{
}


void IVision::Reset()
{
	INextBotComponent::Reset();
	
	this->m_KnownEntities.RemoveAll();
	this->m_flLastUpdate = 0.0f;
	this->m_hPrimaryThreat = nullptr;
	
	this->SetFieldOfView(this->GetDefaultFieldOfView());
	
	for (int i = 0; i < 32; ++i) {
		this->m_Timers[i].Invalidate();
	}
}

void IVision::Update()
{
	VPROF_BUDGET("IVision::Update", "NextBotExpensive");
	
	if (nb_blind.GetBool()) {
		this->m_KnownEntities.RemoveAll();
	} else {
		this->UpdateKnownEntities();
		this->m_flLastUpdate = gpGlobals->curtime;
	}
}


bool IVision::ForEachKnownEntity(IForEachKnownEntity& functor)
{
	FOR_EACH_VEC(this->m_KnownEntities, i) {
		CKnownEntity& known = this->m_KnownEntities[i];
		
		if (!known.IsObsolete() &&
			known->GetTimeSinceBecameKnown() >= this->GetMinRecognizeTime()) {
			if (!functor->Inspect(known)) {
				return false;
			}
		}
	}
	
	return true;
}

void IVision::CollectKnownEntities(CUtlVector<CKnownEntity> *knowns)
{
	if (knowns == nullptr) {
		return;
	}
	
	knowns->RemoveAll();
	knowns->AddVectorToTail(this->m_KnownEntities);
}

const CKnownEntity *IVision::GetPrimaryKnownThreat(bool only_recently_visible) const
{
	if (this->m_KnownEntities.IsEmpty()) {
		return nullptr;
	}
	
	const CKnownEntity *threat = nullptr;
	FOR_EACH_VEC(this->m_KnownEntities, i) {
		const CKnownEntity& known = this->m_KnownEntities[i];
		
		if (known.IsObsolete()) {
			continue;
		}
		
		if (known.GetTimeSinceBecameKnown() < this->GetMinRecognizeTime()) {
			continue;
		}
		
		if (this->IsIgnored(known.GetEntity()) ||
			!this->GetBot()->IsEnemy(known.GetEntity())) {
			continue;
		}
		
		if (only_recently_visible && !known.IsVisibleRecently()) {
			continue;
		}
		
		if (threat == nullptr) {
			threat = &known;
		} else {
			threat = this->GetBot()->GetIntentionInterface()->SelectMoreDangerousThreat(this->GetBot(),
				this->GetBot()->GetEntity(), threat, &known);
		}
	}
	
	if (threat != nullptr) {
		this->m_hPrimaryThreat = threat->GetEntity();
	} else {
		this->m_hPrimaryThreat = nullptr;
	}
	
	return threat;
}

float IVision::GetTimeSinceVisible(int teamnum) const
{
	if (teamnum == -2) {
		for (int i = 0; i < 32; ++i) {
			IntervalTimer *timer = this->m_Timers + i;
			if (timer->IsLessThen(1e+10f) && timer->HasStarted()) {
				/* this code doesn't make any sense, it's probably unfinished */
			}
		}
		return 1e+10f;
	} else if (teamnum < 32) {
		return this->m_Timers[teamnum].GetElapsedTime();
	} else {
		return 0.0f;
	}
}

const CKnownEntity *IVision::GetClosestKnown(int teamnum) const
{
	CKnownEntity *closest = nullptr;
	float dist = 1e+9f;
	
	Vector& pos_me = this->GetBot()->GetPosition();
	
	FOR_EACH_VEC(this->m_KnownEntities, i) {
		const CKnownEntity& known = this->m_KnownEntities[i];
		
		if (known.IsObsolete()) {
			continue;
		}
		
		if (known.GetTimeSinceBecameKnown() < this->GetMinRecognizeTime()) {
			continue;
		}
		
		if (teamnum != TEAM_ANY &&
			(teamnum != known.GetEntity->GetTeamNumber())) {
			continue;
		}
		
		Vector& pos_them = known.GetLastKnownPosition();
		if (pos_me.DistTo(pos_them) < dist) {
			dist = pos_me.DistTo(pos_them);
			closest = &known;
		}
	}
	
	return closest;
}

int IVision::GetKnownCount(int teamnum, bool only_recently_visible, float range) const
{
	int count = 0;
	
	FOR_EACH_VEC(this->m_KnownEntities, i) {
		CKnownEntity& known = this->m_KnownEntities[i];
		
		if (known.IsObsolete()) {
			continue;
		}
		
		if (known.GetTimeSinceBecameKnown() < this->GetMinRecognizeTime()) {
			continue;
		}
		
		if (teamnum != TEAM_ANY &&
			(teamnum != known.GetEntity->GetTeamNumber())) {
			continue;
		}
		
		if (only_recently_visible && !known.IsVisibleRecently()) {
			continue;
		}
		
		if (range >= 0.0f) {
			if (this->GetBot()->IsRangeLessThan(known.GetLastKnownPosition(), range)) {
				++count;
			}
		}
	}
	
	return count;
}

const CKnownEntity *IVision::GetClosestKnown(const INextBotEntityFilter& filter) const
{
	CKnownEntity *closest = nullptr;
	float dist = 1e+9f;
	
	Vector& pos_me = this->GetBot()->GetPosition();
	
	FOR_EACH_VEC(this->m_KnownEntities, i) {
		const CKnownEntity& known = this->m_KnownEntities[i];
		
		if (known.IsObsolete()) {
			continue;
		}
		
		if (known.GetTimeSinceBecameKnown() < this->GetMinRecognizeTime()) {
			continue;
		}
		
		if (!filter.IsAllowed(known.GetEntity())) {
			continue;
		}
		
		Vector& pos_them = known.GetLastKnownPosition();
		if (pos_me.DistTo(pos_them) < dist) {
			dist = pos_me.DistTo(pos_them);
			closest = &known;
		}
	}
	
	return closest;
}

const CKnownEntity *IVision::GetKnown(const CBaseEntity *ent) const
{
	if (ent == nullptr) {
		return nullptr;
	}
	
	FOR_EACH_VEC(this->m_KnownEntities, i) {
		const CKnownEntity& known = this->m_KnownEntities[i];
		
		if (known.GetEntity() != nullptr) {
			CBaseEntity *k_ent = known.GetEntity();
			if (k_ent == ent && !known.IsObsolete()) {
				return &known;
			}
		}
	}
	
	return nullptr;
}

void IVision::AddKnownEntity(CBaseEntity *ent)
{
	CKnownEntity known(ent);
	
	if (!this->m_KnownEntities.HasElement(known)) {
		this->m_KnownEntities.AddToTail(known);
	}
}

void IVision::ForgetEntity(CBaseEntity *ent)
{
	if (ent == nullptr) {
		return;
	}
	
	FOR_EACH_VEC(this->m_KnownEntities, i) {
		CKnownEntity& known = this->m_KnownEntities[i];
		
		if (known.GetEntity() != nullptr) {
			CBaseEntity *k_ent = known.GetEntity();
			if (k_ent == ent && !known.IsObsolete()) {
				this->m_KnownEntities.Remove(i);
				break;
			}
		}
	}
}

void IVision::ForgetAllKnownEntities()
{
	this->m_KnownEntities.RemoveAll();
}

void IVision::CollectPotentiallyVisibleEntities(CUtlVector<CBaseEntity *> *ents)
{
	ForEachActor([&](CBaseCombatCharacter *them) {
			ents->AddToTail(them);
		});
}

float IVision::GetMaxVisionRange() const
{
	return 2000.0f;
}

float IVision::GetMinRecognizeTime() const
{
	return 0.0f;
}

bool IVision::IsAbleToSee(CBaseEntity *ent, FieldOfViewCheckType ctype, Vector *v1) const
{
	VPROF_BUDGET("IVision::IsAbleToSee", "NextBotExpensive");
	
	if (this->GetBot()->IsRangeGreaterThan(ent, this->GetMaxVisionRange()) ||
		this->GetBot()->GetEntity()->IsHiddenByFog(ent)) {
		return false;
	}
	
	if (ctype == FieldOfViewCheckType::USE_FOV && !this->IsInFieldOfView(ent)) {
		return false;
	}
	
	CBaseCombatCharacter *ent_cc = ent->MyCombatCharacterPointer();
	if (ent_cc != nullptr) {
		CTFNavArea *lastknown_ent  = ent_cc->GetLastKnownArea();
		CTFNavArea *lastknown_this =
			this->GetBot()->GetEntity()->GetLastKnownArea();
			
		if (lastknown_ent != nullptr && lastknown_this != nullptr &&
			!lastknown_this->IsPotentiallyVisible(lastknown_ent)) {
			return false;
		}
	}
	
	return (this->IsLineOfSightClearToEntity(ent, nullptr) &&
		this->IsVisibleEntityNoticed(ent));
}

bool IVision::IsAbleToSee(const Vector& vec, FieldOfViewCheckType ctype) const
{
	VPROF_BUDGET("IVision::IsAbleToSee", "NextBotExpensive");
	
	if (this->GetBot()->IsRangeGreaterThan(vec, this->GetMaxVisionRange()) ||
		this->GetBot()->GetEntity()->IsHiddenByFog(vec)) {
		return false;
	}
	
	if (ctype == FieldOfViewCheckType::USE_FOV && !this->IsInFieldOfView(vec)) {
		return false;
	}
	
	return this->IsLineOfSightClear(vec);
}

bool IVision::IsIgnored(CBaseEntity *ent) const
{
	return false;
}

bool IVision::IsVisibleEntityNoticed(CBaseEntity *ent) const
{
	return true;
}

bool IVision::IsInFieldOfView(const Vector& vec) const
{
	Vector *view_vec = this->GetBot()->GetBodyInterface()->GetViewVector();
	Vector *eye_pos  = this->GetBot()->GetBodyInterface()->GetEyePosition();
	
	return PointWithinViewAngle(eye_pos, vec, view_vec, this->m_flCosHalfFOV);
}

bool IVision::IsInFieldOfView(CBaseEntity *ent) const
{
	return (this->IsInFieldOfView(ent->WorldSpaceCenter()) ||
		this->IsInFieldOfView(ent->EyePosition())); 
}

float IVision::GetDefaultFieldOfView() const
{
	return 90.0f;
}

float IVision::GetFieldOfView() const
{
	return this->m_flFOV;
}

void IVision::SetFieldOfView(float fov)
{
	this->m_flFOV = fov;
	this->m_flCosHalfFOV = cosf(RAD2DEG(fov * 0.5f));
}

bool IVision::IsLineOfSightClear(const Vector& v1) const
{
	VPROF_BUDGET("IVision::IsLineOfSightClear", "NextBot");
	VPROF_INCREMENT_COUNTER("IVision::IsLineOfSightClear", 1);
	
	NextBotVisionTraceFilter filter(this->GetBot()->GetEntity());
	trace_t trace;
	
	UTIL_TraceLine(this->GetBot()->GetBodyInterface->GetEyePosition(),
		v1, MASK_VISIBLE_AND_NPCS, &filter, &trace);
	
	return (trace.fraction >= 1.0f && !trace.startsolid);
}

bool IVision::IsLineOfSightClearToEntity(const CBaseEntity *ent, Vector *v1) const
{
	VPROF_BUDGET("IVision::IsLineOfSightClearToEntity", "NextBot");
	
	NextBotTraceFilterIgnoreActors filter(ent);
	trace_t trace;
	
	UTIL_TraceLine(this->GetBot()->GetBodyInterface()->GetEyePosition(),
		ent->WorldSpaceCenter(), MASK_VISIBLE_AND_NPCS, &filter, &trace);
	
	if (trace.fraction < 1.0f || trace.allsolid || trace.startsolid) {
		UTIL_TraceLine(this->GetBot()->GetBodyInterface()->GetEyePosition(),
			ent->EyePosition(), MASK_VISIBLE_AND_NPCS, &filter, &trace);
		
		if (trace.fraction < 1.0f || trace.allsolid || trace.startsolid) {
			UTIL_TraceLine(this->GetBot()->GetBodyInterface()->GetEyePosition(),
				ent->GetAbsOrigin(), MASK_VISIBLE_AND_NPCS, &filter, &trace);
		}
	}
	
	if (v1 != nullptr) {
		v1 = trace.endpos;
	}
	
	return (trace.fraction >= 1.0f && !trace.startsolid);
}

bool IVision::IsLookingAt(const Vector& v1, float cos_half_fov) const
{
	Vector *view_vec = this->GetBot()->GetBodyInterface()->GetViewVector();
	Vector *eye_pos  = this->GetBot()->GetBodyInterface()->GetEyePosition();
	
	return PointWithinViewAngle(eye_pos, v1, view_vec, cos_half_fov);
}

bool IVision::IsLookingAt(const CBaseCombatCharacter *who, float cos_half_fov) const
{
	return this->IsLookingAt(who->EyePosition(), cos_half_fov);
}


void IVision::UpdateKnownEntities()
{
	VPROF_BUDGET("IVision::UpdateKnownEntities", "NextBot");
	
	CUtlVector<CBaseEntity *> vec_maybe_visible;
	this->CollectPotentiallyVisibleEntities(&vec_maybe_visible);
	
	CUtlVector<CBaseEntity *> vec_visble;
	
	FOR_EACH_VEC(vec_maybe_visible, i) {
		VPROF_BUDGET("IVision::UpdateKnownEntities( collect visible )", "NextBot");
		
		CBaseEntity *ent = vec_maybe_visible[i];
		if (ent == nullptr) {
			continue;
		}
		
		if (this->IsIgnored(ent) || !ent->IsAlive()) {
			continue;
		}
		
		if (ent == this->GetBot()->GetEntity() ||
			!this->IsAbleToSee(ent, FieldOfViewCheckType::USE_FOV, nullptr)) {
			continue;
		}
		
		vec_visible.AddToTail(ent);
	}
	
	{
		VPROF_BUDGET("IVision::UpdateKnownEntities( update status )", "NextBot");
		
		FOR_EACH_VEC(this->m_KnownEntities, i) {
			CKnownEntity& known = this->m_KnownEntities[i];
			
			if (known.GetEntity() == nullptr || known.IsObsolete()) {
				this->m_KnownEntities.Remove(i);
				--i;
				continue;
			}
			
			bool known_is_visible = false;
			FOR_EACH_VEC(vec_visible, j) {
				if (ENTINDEX(known.GetEntity()) == ENTINDEX(vec_visible[j])) {
					known_is_visible = true;
					break;
				}
			}
			
			if (known_is_visible) {
				known.UpdatePosition();
				known.UpdateVisibilityStatus(true);
				
				if (known.GetTimeSinceBecameVisible() >= this->GetMinRecognizeTime()) {
					if ((this->m_flLastUpdate - known.GetTimeWhenBecameVisible()) <= this->GetMinRecognizeTime()) {
						if (this->GetBot()->IsDebugging(DEBUG_VISION)) {
							ConColorMsg(COLOR_GREEN, "%3.2f: %s caught sight of %s(#%d)\n",
								gpGlobals->curtime, this->GetBot->GetDebugIdentifier(),
								known.GetEntity()->GetClassname(), ENTINDEX(known.GetEntity());
							
							NDebugOverlay::Line(this->GetBot()->GetBodyInterface()->GetEyePosition(),
								known.GetLastKnownPosition(), RGB_YELLOW, false, 0.2f);
						}
						
						this->GetBot()->OnSight(known.GetEntity());
					}
				}
				
				this->m_Timers[known.GetEntity()->GetTeamNumber()].Start(0.0f);
			} else {
				if (known.IsVisibleInFOVNow()) {
					known.UpdateVisibilityStatus(false);
					
					if (this->GetBot()->IsDebugging(DEBUG_VISION)) {
						ConColorMsg(COLOR_RED, "%3.2f: %s lost sight of %s(#%d)\n",
							gpGlobals->curtime, this->GetBot->GetDebugIdentifier(),
							known.GetEntity()->GetClassname(), ENTINDEX(known.GetEntity());
					}
					
					this->GetBot()->OnLostSight(known.GetEntity());
				}
				
				if (!known.HasLastKnownPositionBeenSeen() &&
					this->IsAbleToSee(known.GetLastKnownPosition(),
						FieldOfViewCheckType::USE_FOV)) {
					known.MarkLastKnownPositionAsSeen();
				}
			}
		}
	}
	
	{
		VPROF_BUDGET("IVision::UpdateKnownEntities( new recognizes )", "NextBot");
		
		FOR_EACH_VEC(vec_visible, i) {
			CBaseEntity *visible = vec_visible[i];
			
			bool visible_already_known = false;
			FOR_EACH_VEC(this->m_KnownEntities, j) {
				CKnownEntity& known = this->m_KnownEntities[j];
				
				if (known.GetEntity() == visible) {
					visible_already_known = true;
					break;
				}
			}
			
			if (visible_already_known) {
				continue;
			}
			
			CKnownEntity k_new(visible);
			k_new.UpdateVisibilityStatus(true);
			this->m_KnownEntities.AddToTail(k_new);
		}
	}
	
	if (nb_debug_known_entities.GetBool()) {
		CBasePlayer *host = UTIL_GetListenServerHost();
		if (host != nullptr) {
			CBaseEntity *spec_target = host->GetObserverTarget();
			if (this->GetBot()->IsSelf(spec_target)) {
				CUtlVector<CKnownEntity> vec_known;
				this->CollectKnownEntities(&vec_known);
				
				FOR_EACH_VEC(vec_known, i) {
					CKnownEntity& known = vec_known[i];
					
					bool is_friend = this->GetBot()->IsFriend(known.GetEntity());
					bool is_recognized = (known.GetTimeSinceBecameKnown() >= this->GetMinRecognizeTime());
					bool is_visible_now = known.IsVisibleInFOVNow();
					
					float width;
					bool bright;
					
					if (is_recognized) {
						if (is_visible_now) {
							width = 5.0f;
							bright = true;
						} else {
							width = 2.0f;
							bright = false;
						}
					} else {
						width = 1.0f;
						bright = false;
					}
					
					if (is_friend) {
						r = 0;
						g = (bright ? 255 : 100);
						b = 0;
					} else {
						r = (bright ? 255 : 100);
						g = 0;
						b = 0;
					}
					
					NDebugOverlay::HorzArrow(this->GetEntity()->GetAbsOrigin(),
						known.GetLastKnownPosition(), width,
						r, g, b, 255, true, 0.0f);
				}
			}
		}
	}
}
