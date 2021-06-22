/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/NextBotLocomotionInterface.cpp
 */


ILocomotion::ILocomotion(INextBot *nextbot)
	: INextBotComponent(nextbot);
{
	// TODO
	
	this->Reset();
}

ILocomotion::~ILocomotion()
{
	// TODO
}


void ILocomotion::OnLeaveGround(CBaseEntity *ent)
{
}

void ILocomotion::OnLandOnGround(CBaseEntity *ent)
{
}


void ILocomotion::Reset()
{
	INextBotComponent::Reset();
	
	// TODO
}

void ILocomotion::Update()
{
	// TODO
}


void ILocomotion::Approach(const Vector& v1, float f1)
{
	this->m_itApproach.Start();
}

void ILocomotion::DriveTo(const Vector& v1)
{
	this->m_itApproach.Start();
}

bool ILocomotion::ClimbUpToLedge(const Vector& v1, const Vector& v2, const CBaseEntity *ent)
{
	return true;
}

void ILocomotion::JumpAcrossGap(const Vector& v1, const Vector& v2)
{
}

void ILocomotion::Jump()
{
}

bool ILocomotion::IsClimbingOrJumping() const
{
	return false;
}

bool ILocomotion::IsClimbingUpToLedge() const
{
	return false;
}

bool ILocomotion::IsJumpingAcrossGap() const
{
	return false;
}

bool ILocomotion::IsScrambling() const
{
	if (!this->IsOnGround() || this->IsClimbingOrJumping()) {
		return true;
	} else {
		return this->IsAscendingOrDescendingLadder();
	}
}

void ILocomotion::Run()
{
}

void ILocomotion::Walk()
{
}

void ILocomotion::Stop()
{
}

bool ILocomotion::IsRunning() const
{
	return false;
}

void ILocomotion::SetDesiredSpeed(float speed)
{
}

float ILocomotion::GetDesiredSpeed() const
{
	return 0.0f;
}

void ILocomotion::SetSpeedLimit(float limit)
{
}

float ILocomotion::GetSpeedLimit() const
{
	return 1000.0f;
}

bool ILocomotion::IsOnGround() const
{
	return false;
}

CBaseEntity *ILocomotion::GetGround() const
{
	return nullptr;
}

Vector& ILocomotion::GetGroundNormal() const
{
	return vec3_origin;
}

float ILocomotion::GetGroundSpeed() const
{
	return this->m_flGroundSpeed;
}

Vector& ILocomotion::GetGroundMotionVector() const
{
	return this->m_vecGroundMotion;
}

void ILocomotion::ClimbLadder(const CNavLadder *ladder, const CNavArea *area)
{
}

void ILocomotion::DescendLadder(const CNavLadder *ladder, const CNavArea *area)
{
}

bool ILocomotion::IsUsingLadder() const
{
	return false;
}

bool ILocomotion::IsAscendingOrDescendingLadder() const
{
	return false;
}

bool ILocomotion::IsAbleToAutoCenterOnLadder() const
{
	return false;
}

void ILocomotion::FaceTowards(const Vector& v1)
{
}

void ILocomotion::SetDesiredLean(const QAngle& q1)
{
}

QAngle& ILocomotion::GetDesiredLean() const
{
	return vec3_angle;
}

bool ILocomotion::IsAbleToJumpAcrossGaps() const
{
	return true;
}

bool ILocomotion::IsAbleToClimb() const
{
	return true;
}

Vector& ILocomotion::GetFeet() const
{
	return this->GetBot()->GetEntity()->GetAbsOrigin();
}

float ILocomotion::GetStepHeight() const
{
	return 0.0f;
}

float ILocomotion::GetMaxJumpHeight() const
{
	return 0.0f;
}

float ILocomotion::GetDeathDropHeight() const
{
	return 0.0f;
}

float ILocomotion::GetRunSpeed() const
{
	return 0.0f;
}

float ILocomotion::GetWalkSpeed() const
{
	return 0.0f;
}

float ILocomotion::GetMaxAcceleration() const
{
	return 0.0f;
}

float ILocomotion::GetMaxDeceleration() const
{
	return 0.0f;
}

Vector& ILocomotion::GetVelocity() const
{
	return vec3_origin;
}

float ILocomotion::GetSpeed() const
{
	return this->m_flSpeed;
}

Vector& ILocomotion::GetMotionVector() const
{
	return this->m_vecMotion;
}

bool ILocomotion::IsAreaTraversable(const CNavArea *area) const
{
	// TODO
}

float ILocomotion::GetTraversableSlopeLimit() const
{
	return 0.6f;
}

bool ILocomotion::IsPotentiallyTraversable(const Vector& v1, const Vector& v2, TraverseWhenType ttype, float *f1) const
{
	VPROF_BUDGET("Locomotion::IsPotentiallyTraversable", "NextBotExpensive");
	
	float dz = v2->z - v2->z;
	if (dz > this->GetMaxJumpHeight() + 0.1f) {
		Vector dv = v2 - v1;
		float dv_len_sqr = dv.LengthSqr();
		float dv_len_inv = 1.0f / fsqrt(dv_len_sqr);
		
		if ((dv_len_inv * ((3.0f - ((dv_len_inv * dv_len_inv) * dv_len_sqr)) *
			0.5f)) * dz > this->GetTraversableSlopeLimit()) {
			if (f1 != nullptr) {
				*f1 = 0.0f;
			}
			return false;
		}
	}
	
	NextBotTraversableTraceFilter filter(this->GetBot()->GetEntity());
	
	// TODO
}

bool ILocomotion::HasPotentialGap(const Vector& v1, const Vector& v2, float *f1) const
{
	VPROF_BUDGET("Locomotion::HasPotentialGap", "NextBot");
	
	// TODO
}

bool ILocomotion::IsGap(const Vector& v1, const Vector& v2) const
{
	VPROF_BUDGET("Locomotion::IsGap", "NextBotSpiky");
	
	unsigned int mask;
	IBody *body = this->GetBot()->GetBodyInterface();
	if (body != nullptr) {
		mask = body->GetSolidMask();
	} else {
		mask = MASK_PLAYERSOLID;
	}
	
	NextBotTraceFilterIgnoreActors filter(this->GetBot()->GetEntity());
	
	Vector v_start = v1;
	Vector v_end   = {
		.x = v1.x,
		.y = v1.y,
		.z = v1.z + this->GetStepHeight(),
	};
	Vector v_mins = {
		.x = 0.0f,
		.y = 0.0f,
		.z = 0.0f,
	};
	Vector v_maxs = {
		.x = 1.0f,
		.y = 1.0f,
		.z = 1.0f,
	};
	
	Ray_t ray;
	ray.Init(v_start, v_end, v_mins, v_maxs);
	
	trace_t trace;
	enginetrace->TraceRay(ray, mask, &filter, &trace);
	
	return (trace.fraction < 1.0 && !trace.startsolid);
}

bool ILocomotion::IsEntityTraversable(CBaseEntity *ent, TraverseWhenType ttype) const
{
	// TODO
}

bool ILocomotion::IsStuck() const
{
	return this->m_bIsStuck;
}

float ILocomotion::GetStuckDuration() const
{
	if (this->IsStuck()) {
		return this->m_itStuck.GetElapsedTime();
	} else {
		return 0.0f;
	}
}

void ILocomotion::ClearStuckStatus(char const *s1)
{
	if (this->IsStuck()) {
		this->m_bIsStuck = false;
		this->OnUnStuck();
	}
	
	this->m_vecStuck = this->GetFeet();
	
	this->m_itStuck.Start();
	
	if (this->GetBot()->IsDebugging(DEBUG_LOCOMOTION)) {
		DevMsg("%3.2f: ClearStuckStatus: %s %s\n",
			gpGlobals->curtime, this->GetBot()->GetDebugIdentifier(), s1);
	}
}

bool ILocomotion::IsAttemptingToMove() const
{
	if (this->m_itApproach.HasStarted()) {
		return this->m_itApproach.IsLessThen(0.25f);
	} else {
		return false;
	}
}

bool ILocomotion::ShouldCollideWith(const CBaseEntity *ent) const
{
	return true;
}

void ILocomotion::AdjustPosture(const Vector& v1)
{
	// TODO
}

void ILocomotion::StuckMonitor()
{
	// TODO: this function needs significant refactoring
	// extract some of these if statements into bools and flatten it out
	
	if (this->m_itApproach.IsGreaterThen(0.25f)) {
		this->m_vecStuck = this->GetFeet();
		this->m_itStuck.Start();
	} else {
		if (this->IsStuck()) {
			if (this->GetBot()->IsRangeGreaterThan(this->m_vecStuck, 100.0f)) {
				this->ClearStuckStatus("UN-STUCK");
			} else {
				if (this->m_ctUnknown.IsElapsed()) {
					this->m_ctUnknown.Start(1.0f);
					if (this->GetBot()->IsDebugging(DEBUG_LOCOMOTION)) {
						// TODO
					}
					
					this->GetBot()->OnStuck();
				}
			}
		} else {
			if (this->GetBot()->IsRangeGreaterThan(this->m_vecStuck, 100.0f)) {
				this->m_vecStuck = this->GetFeet();
				if (this->GetBot()->IsDebugging(DEBUG_LOCOMOTION)) {
					// TODO: Cross3D
				}
				this->m_itStuck.Start();
			} else {
				if (this->GetBot()->IsDebugging(DEBUG_LOCOMOTION)) {
					// TODO
				}
				
				// TODO
			}
		}
	}
}


void ILocomotion::TraceHull(const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, unsigned int mask, ITraceFilter *filter, CGameTrace *trace)
{
	Ray_t ray;
	ray.Init(start + maxs, end + maxs, mins, maxs);
	
	enginetrace->TraceRay(ray, mask, filter, trace);
}
