/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/Path/NextBotChasePath.cpp
 */


ChasePath::ChasePath()
{
	// TODO
}

ChasePath::~ChasePath()
{
}


void ChasePath::Invalidate()
{
	this->m_ctTimer2.Invalidate();
	this->m_ctTimer3.Invalidate();
	
	PathFollower::Invalidate();
}


void ChasePath::Update(INextBot *nextbot, CBaseEntity *ent, const IPathCost& cost_func, Vector *vec)
{
	VPROF_BUDGET("ChasePath::Update", "NextBot");
	
	this->RefreshPath(nextbot, ent, cost_func, vec);
	PathFollower::Update(nextbot);
}

float ChasePath::GetLeadRadius() const
{
	return 500.0f;
}

float ChasePath::GetMaxPathLength() const
{
	return 0.0f;
}

Vector ChasePath::PredictSubjectPosition(INextBot *nextbot, CBaseEntity *ent) const
{
	ILocomotion *loco = nextbot->GetLocomotionInterface();
	
	const Vector& bot_pos = nextbot->GetPosition();
	const Vector& ent_pos = ent->GetAbsOrigin();
	
	Vector2D bot_to_ent = ent_pos.AsVector2D() - bot_pos.AsVector2D();
	
	float lead_radius = loco->GetLeadRadius();
	if (bot_to_ent.IsLengthGreaterThan(lead_radius)) {
		return ent_pos;
	}
	
	float dist_to_ent = bot_to_ent.Length();
	bot_to_ent.NormalizeInPlace();
	
	float run_speed = loco->GetRunSpeed();
	float time_to_ent = (dist_to_ent / run_speed) + 0.5f;
	
	Vector2D pred_ent_dpos = time_to_ent * ent->GetAbsVelocity().AsVector2D();
	
	/* handle cases where entity is moving toward us rather than away */
	if (pred_ent_dpos.Dot(bot_to_ent) < 0.0f) {
		// TODO: tricky math in here
		// seems to be mirroring pred_ent_dpos about bot_to_ent
		// (effectively changing "right-side" to "left-side" and vice versa)
	}
	
	Vector pred_ent_pos = {
		.x = ent_pos.x + pred_ent_dpos.x,
		.y = ent_pos.y + pred_ent_dpos.y,
		.z = ent_pos.z,
	};
	
	if (pred_ent_dpos.IsLengthGreaterThan(6.0f)) {
		// TODO: enum name for (ILocomotion::TraverseWhenType)0
		// TODO: useful name for f1 from ILocomotion::IsPotentiallyTraversible
		float f1;
		if (!loco->IsPotentiallyTraversible(ent_pos, pred_ent_pos,
			(ILocomotion::TraverseWhenType)0, &f1)) {
			/* scale our prediction amount to f1 */
			pred_ent_pos = ent_pos + (f1 * (pred_ent_pos - ent_pos));
		}
	}
	
	CNavArea *pred_area = TheNavMesh->GetNearestNavArea(pred_ent_pos);
	if (pred_area == nullptr) {
		return ent_pos;
	}
	
	float pred_area_z = pred_area->GetZ(pred_ent_pos.x, pred_ent_pos.y);
	if (pred_area_z + loco->GetMaxJumpHeight() < pred_ent_pos.z) {
		return ent_pos;
	}
	
	return pred_ent_pos;
}

bool ChasePath::IsRepathNeeded(INextBot *nextbot, CBaseEntity *ent) const
{
	float dist_nb_to_ent =
		nextbot->GetPosition().DistTo(ent->GetAbsOrigin());
	float dist_endpos_to_ent =
		this->GetEndPosition().DistTo(ent->GetAbsOrigin());
	
	return (dist_endpos_to_ent > (dist_nb_to_ent * (1.0f / 3.0f)));
}

float ChasePath::GetLifetime() const
{
	return 0.0f;
}


void ChasePath::RefreshPath(INextBot *nextbot, CBaseEntity *ent, const IPathCost& cost_func, Vector *vec)
{
	VPROF_BUDGET("ChasePath::RefreshPath", "NextBot");
	
	if (this->IsValid() && nextbot->GetLocomotionInterface()->IsUsingLadder()) {
		if (nextbot->IsDebugging(DEBUG_PATH)) {
			DevMsg("%3.2f: bot(#%d) ChasePath::RefreshPath failed. Bot is on a ladder.\n",
				gpGlobals->curtime, ENTINDEX(nextbot->GetEntity()));
		}
		
		this->m_ctTimer2.Start(1.0f);
		return;
	}
	
	if (ent == nullptr) {
		if (nextbot->IsDebugging(DEBUG_PATH)) {
			/* misspelling is in original */
			DevMsg("%3.2f: bot(#%d) CasePath::RefreshPath failed. No subject.\n",
				gpGlobals->curtime, ENTINDEX(nextbot->GetEntity()));
		}
	}
	
	if (this->m_ctTimer1.IsElapsed()) {
		CBaseEntity *subject = this->m_hChaseSubject();
		if (subject == nullptr || subject != ent) {
			if (nextbot->IsDebugging(DEBUG_PATH)) {
				DevMsg("%3.2f: bot(#%d) ChasePath::RefreshPath subject changed (from %p to %p).\n",
					gpGlobals->curtime, ENTINDEX(nextbot->GetEntity()),
					this->m_hChaseSubject(), ent);
			}
			
			this->Invalidate();
			this->m_ctTimer1.Invalidate();
		}
		
		if (!this->IsValid() || this->m_ctTimer2.IsElapsed()) {
			if (this->IsValid() && this->m_ctTimer3.HasStarted() &&
				this->m_ctTimer3.IsElapsed()) {
				this->Invalidate();
			}
			
			if (!this->IsValid() || this->IsRepathNeeded(nextbot, ent)) {
				// TODO
				
				
				// eventually: this->Compute(...)
			}
			
			// TODO
		}
		
		// TODO
	}
}
