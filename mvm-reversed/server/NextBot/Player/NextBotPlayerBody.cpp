/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/Player/NextBotPlayerBody.cpp
 */


ConVar nb_saccade_time("nb_saccade_time", "0.1", FCVAR_CHEAT);
ConVar nb_saccade_speed("nb_saccade_speed", "1000", FCVAR_CHEAT);
ConVar nb_head_aim_steady_max_rate("nb_head_aim_steady_max_rate", "100", FCVAR_CHEAT);
ConVar nb_head_aim_settle_duration("nb_head_aim_settle_duration", "0.3", FCVAR_CHEAT);
ConVar nb_head_aim_resettle_angle("nb_head_aim_resettle_angle", "100", FCVAR_CHEAT,
	"After rotating through this angle, the bot pauses to 'recenter' its virtual mouse on its virtual mousepad");
ConVar nb_head_aim_resettle_time("nb_head_aim_resettle_time", "0.3", FCVAR_CHEAT,
	"How long the bot pauses to 'recenter' its virtual mouse on its virtual mousepad");
ConVar bot_mimic("bot_mimic", "0", FCVAR_NONE,
	"Bot uses usercmd of player by index.");


PlayerBody::PlayerBody(INextBot *nextbot)
	: IBody(nextbot)
{
	this->m_Player = static_cast<CBasePlayer *>(nextbot->GetEntity());
}

PlayerBody::~PlayerBody()
{
}


void PlayerBody::Reset()
{
	this->m_iPosture = IBody::PostureType::STAND;
	
	this->m_angLastEyeAngles  = vec3_angle;
	this->m_vecAimTarget      = vec3_origin;
	this->m_vecTargetVelocity = vec3_origin;
	this->m_vecLastEyeVectors = vec3_origin;
	
	this->m_hAimTarget = nullptr;
	this->m_AimReply   = nullptr;
	this->m_iAimPriority = LookAtPriorityType::BORING;
	
	this->m_ctAimDuration.Invalidate();
	this->m_itAimStart.Invalidate();
	this->m_itHeadSteady.Invalidate();
	this->m_ctResettle.Invalidate();
	
	this->m_bHeadOnTarget = false;
	this->m_bSightedIn    = false;
}

void PlayerBody::Upkeep()
{
	static ConVarRef bot_mimic("bot_mimic");
	if (bot_mimic.IsValid() && !bot_mimic.GetBool()) {
		return;
	}
	
	float frametime = gpGlobals->frametime;
	if (gpGlobals->frametime < 1.0e-5f) {
		return;
	}
	
	CBasePlayer *actor = static_cast<CBasePlayer *>(this->GetBot()->GetEntity());
	
	QAngle eye_ang = actor->EyeAngles() + actor->GetPunchAngle();
	
	bool head_steady;
	if (abs((int)AngleDiff(eye_ang.x, this->m_angLastEyeAngles.x)) >
		(frametime * nb_head_aim_steady_max_rate.GetFloat()) ||
		abs((int)AngleDiff(eye_ang.y, this->m_angLastEyeAngles.y)) >
		(frametime * nb_head_aim_steady_max_rate.GetFloat())) {
		this->m_itHeadSteady.Invalidate();
		head_steady = false;
	} else {
		if (!this->m_itHeadSteady.HasStarted()) {
			this->m_itHeadSteady.Start();
		}
		head_steady = true;
	}
	
	if (this->GetBot()->IsDebugging(DEBUG_LOOK_AT) && this->IsHeadSteady()) {
		float radius = Clamp(this->GetHeadSteadyDuration * (1.0f / 3.0f),
			0.0f, 1.0f) * 10.0f;
		NDebugOverlay::Circle(actor->EyePosition(),
			radius, RGBA_GREEN, true, 2 * frametime);
	}
	
	this->m_angLastEyeAngles = eye_ang;
	
	if (this->m_bSightedIn && this->m_ctAimDuration.IsElapsed()) {
		return;
	}
	
	const Vector& eye_vec = this->GetViewVector();
	if (acosf(this->m_vecLastEyeVectors.Dot(eye_vec)) * (180.0f / M_PI) >
		nb_head_aim_resettle_angle.GetFloat()) {
		this->m_ctResettle.Start(nb_head_aim_resettle_time.GetFloat() *
			RandomFloat(0.9f, 1.1f));
		this->m_vecLastEyeVectors = eye_vec;
	} else if (!this->m_ctResettle.HasStarted() || this->m_ctResettle.IsElapsed()) {
		this->m_ctResettle.Invalidate();
		
		CBaseEntity *target_ent = this->m_hAimTarget();
		if (target_ent != nullptr) {
			if (this->m_ctAimTracking.IsElapsed()) {
				Vector target_point;
				if (target_ent->IsCombatCharacter()) {
					target_point = this->GetBot()->GetIntentionInterface(this->GetBot(),
						target_ent->MyCombatCharacterPointer());
				} else {
					target_point = target_ent->WorldSpaceCenter();
				}
				
				Vector delta = target_point - this->m_vecAimTarget +
					(this->GetHeadAimSubjectLeadTime() * target_ent->GetAbsVelocity());
				
				float track_interval = Max(frametime,
					this->GetHeadAimTrackingInterval());
				
				float scale = delta.Length() / track_interval;
				delta.NormalizeInPlace();
				
				this->m_vecTargetVelocity = (scale * delta) +
					target_ent->GetAbsVelocity();
				
				this->m_ctAimTracking.Start(track_interval *
					RandomFloat(0.8f, 1.2f));
			}
			
			this->m_vecAimTarget += frametime * this->m_vecTargetVelocity;
		}
	}
	
	Vector eye_to_target = this->m_vecAimTarget - this->GetEyePosition();
	eye_to_target.NormalizeInPlace();
	
	QAngle ang_to_target;
	VectorAngles(eye_to_target, ang_to_target);
	
	if (this->GetBot()->IsDebugging(DEBUG_LOOK_AT)) {
		NDebugOverlay::Line(this->GetEyePosition(),
			this->GetEyePosition() + (100.0f * eye_vec),
			RGB_YELLOW, false, 2 * frametime);
		
		int r = (this->m_bHeadOnTarget ? 255 : 0);
		int g = (target_ent != nullptr ? 255 : 0);
		int b = 255;
		NDebugOverlay::HorzArrow(this->GetEyePosition(),
			this->m_vecAimTarget, (head_steady ? 2.0f : 3.0f),
			r, g, b, 255, false, 2 * frametime);
	}
	
	float cos_error = eye_to_target.Dot(eye_vec);
	
	/* must be within ~11.5 degrees to be considered on target */
	if (cos_error <= 0.98f) {
		this->m_bHeadOnTarget = false;
	} else {
		this->m_bHeadOnTarget = true;
		
		if (!this->m_bSightedIn) {
			this->m_bSightedIn = true;
			if (this->GetBot()->IsDebugging(DEBUG_LOOK_AT)) {
				ConColorMsg(COLOR_ORANGE, "%3.2f: %s Look At SIGHTED IN\n",
					gpGlobals->curtime, this->m_Player->GetPlayerName());
			}
		}
		
		if (this->m_AimReply != nullptr) {
			this->m_AimReply->OnSuccess(this->GetBot());
			this->m_AimReply = nullptr;
		}
	}
	
	float max_angvel = this->GetMaxHeadAngularVelocity();
	
	/* adjust angular velocity limit based on aim error amount */
	if (cos_error > 0.7f) {
		/* unintentional conversion to double due to using sin, not sinf */
		max_angvel *= sin((3.14f / 2.0f) * (1.0f +
			((-49.0f / 15.0f) * (cos_error - 0.7f))));
	}
	
	/* start turning gradually during the first quarter second */
	if (this->m_itAimStart.HasStarted() && this->m_itAimStart.IsLessThen(0.25f)) {
		max_angvel *= 4.0f * m_itAimStart.GetElapsedTime();
	}
	
	QAngle new_eye_angle = {
		.x = ApproachAngle(ang_to_target.x, eye_ang.x, (max_angvel * frametime) * 0.5f),
		.y = ApproachAngle(ang_to_target.y, eye_ang.y, (max_angvel * frametime)),
		.z = 0.0f,
	};
	
	new_eye_angle -= actor->GetPunchAngle();
	new_eye_angle.x = AngleNormalize(new_eye_angle.x);
	new_eye_angle.y = AngleNormalize(new_eye_angle.y);
	
	actor->SnapEyeAngles(new_eye_angle);
}


bool PlayerBody::SetPosition(const Vector& pos)
{
	this->m_Player->SetAbsOrigin(pos);
	return true;
}

Vector& PlayerBody::GetEyePosition() const
{
	this->m_vecEyePosition = this->m_Player->EyePosition();
	return this->m_vecEyePosition;
}

Vector& PlayerBody::GetViewVector() const
{
	this->m_Player->EyeVectors(&this->m_vecEyeVectors);
	return this->m_vecEyeVectors;
}

void PlayerBody::AimHeadTowards(const Vector& vec, IBody::LookAtPriorityType priority, float duration, INextBotReply *reply, const char *reason)
{
	if (duration <= 0.0f) {
		duration = 0.1f;
	}
	
	if (priority == this->m_iAimPriority &&
		(!this->IsHeadSteady() || this->GetHeadSteadyDuration() <
		nb_head_aim_settle_duration.GetFloat())) {
		if (reply != nullptr) {
			reply->OnFail(this->GetBot(),
				INextBotReply::FailureReason::REJECTED);
		}
		
		if (this->GetBot()->IsDebugging(DEBUG_LOOK_AT)) {
			ConColorMsg(COLOR_RED, "%3.2f: %s Look At '%s' rejected - "
				"previous aim not %s\n",
				gpGlobals->curtime, this->m_Player->GetPlayerName(), reason,
				(this->IsHeadSteady() ? "settled long enough" : "head-steady"));
		}
	}
	
	if (priority > this->m_iAimPriority || this->m_ctAimDuration.IsElapsed()) {
		if (this->m_AimReply != nullptr) {
			this->m_AimReply->OnFail(this->GetBot(),
				INextBotReply::FailureReason::PREEMPTED);
		}
		this->m_AimReply = reply;
		
		this->m_ctAimDuration.Start(duration);
		this->m_iAimPriority = priority;
		
		/* only update our aim if the target vector changed significantly */
		if (vec->DistTo(this->m_vecAimTarget) >= 1.0f) {
			this->m_hAimTarget = nullptr;
			this->m_vecAimTarget = vec;
			this->m_itAimStart.Start();
			this->m_bHeadOnTarget = false;
			
			if (this->GetBot()->IsDebugging(DEBUG_LOOK_AT)) {
				NDebugOverlay::Cross3D(vec,
					2.0f, RGB_LTYELLOW, true, 2 * duration);
				
				const char *pri_str = "";
				switch (priority) {
				case IBody::PriorityType::BORING:
					pri_str = "Boring";
					break;
				case IBody::PriorityType::INTERESTING:
					pri_str = "Interesting";
					break;
				case IBody::PriorityType::IMPORTANT:
					pri_str = "Important";
					break;
				case IBody::PriorityType::CRITICAL:
					pri_str = "Critical";
					break;
				}
				
				ConColorMsg(COLOR_ORANGE, "%3.2f: %s Look At ( %g, %g, %g ) "
					"for %3.2f s, Pri = %s, Reason = %s\n",
					gpGlobals->curtime, this->m_Player->GetPlayerName(),
					vec.x, vec.y, vec.z, duration, pri_str,
					(reason != nullptr ? reason : ""));
			}
		}
	} else {
		if (reply != nullptr) {
			reply->OnFail(this->GetBot(),
				INextBotReply::FailureReason::REJECTED);
		}
		
		if (this->GetBot()->IsDebugging(DEBUG_LOOK_AT)) {
			ConColorMsg(COLOR_RED, "%3.2f: %s Look At '%s' rejected - "
				"higher priority aim in progress\n",
				gpGlobals->curtime, this->m_Player->GetPlayerName(), reason);
		}
	}
}

void PlayerBody::AimHeadTowards(CBaseEntity *ent, IBody::LookAtPriorityType priority, float duration, INextBotReply *reply, const char *reason)
{
	if (duration <= 0.0f) {
		duration = 0.1f;
	}
	
	if (priority == this->m_iAimPriority &&
		(!this->IsHeadSteady() || this->GetHeadSteadyDuration() <
		nb_head_aim_settle_duration.GetFloat())) {
		if (reply != nullptr) {
			reply->OnFail(this->GetBot(),
				INextBotReply::FailureReason::REJECTED);
		}
		
		if (this->GetBot()->IsDebugging(DEBUG_LOOK_AT)) {
			ConColorMsg(COLOR_RED, "%3.2f: %s Look At '%s' rejected - "
				"previous aim not %s\n",
				gpGlobals->curtime, this->m_Player->GetPlayerName(), reason,
				(this->IsHeadSteady() ? "settled long enough" : "head-steady"));
		}
	}
	
	if (priority > this->m_iAimPriority || this->m_ctAimDuration.IsElapsed()) {
		if (this->m_AimReply != nullptr) {
			this->m_AimReply->OnFail(this->GetBot(),
				INextBotReply::FailureReason::PREEMPTED);
		}
		this->m_AimReply = reply;
		
		this->m_ctAimDuration.Start(duration);
		this->m_iAimPriority = priority;
		
		/* only update our aim if the target entity changed */
		CBaseEntity *prev_target = this->m_hAimTarget();
		if (prev_target == nullptr || ent != prev_target) {
			this->m_hAimTarget = ent;
			this->m_itAimStart.Start();
			this->m_bHeadOnTarget = false;
			
			if (this->GetBot()->IsDebugging(DEBUG_LOOK_AT)) {
				NDebugOverlay::Cross3D(this->m_vecAimTarget,
					2.0f, RGB_GRAY100, true, duration);
				
				const char *pri_str = "";
				switch (priority) {
				case IBody::PriorityType::BORING:
					pri_str = "Boring";
					break;
				case IBody::PriorityType::INTERESTING:
					pri_str = "Interesting";
					break;
				case IBody::PriorityType::IMPORTANT:
					pri_str = "Important";
					break;
				case IBody::PriorityType::CRITICAL:
					pri_str = "Critical";
					break;
				}
				
				ConColorMsg(COLOR_ORANGE, "%3.2f: %s Look At subject %s "
					"for %3.2f s, Pri = %s, Reason = %s\n",
					gpGlobals->curtime, this->m_Player->GetPlayerName(),
					ent->GetClassname(), duration, pri_str,
					(reason != nullptr ? reason : ""));
			}
		}
	} else {
		if (reply != nullptr) {
			reply->OnFail(this->GetBot(),
				INextBotReply::FailureReason::REJECTED);
		}
		
		if (this->GetBot()->IsDebugging(DEBUG_LOOK_AT)) {
			ConColorMsg(COLOR_RED, "%3.2f: %s Look At '%s' rejected - "
				"higher priority aim in progress\n",
				gpGlobals->curtime, this->m_Player->GetPlayerName(), reason);
		}
	}
}

bool PlayerBody::IsHeadAimingOnTarget() const
{
	return this->m_bHeadOnTarget;
}

bool PlayerBody::IsHeadSteady() const
{
	return this->m_itHeadSteady.HasStarted();
}

float PlayerBody::GetHeadSteadyDuration() const
{
	if (!this->m_itHeadSteady.HasStarted()) {
		return 0.0f;
	}
	
	return this->m_itHeadSteady.GetElapsedTime();
}

void PlayerBody::ClearPendingAimReply()
{
	this->m_AimReply = nullptr;
}

float PlayerBody::GetMaxHeadAngularVelocity() const
{
	return nb_saccade_speed.GetFloat();
}

bool PlayerBody::StartActivity(Activity a1, unsigned int i1)
{
	return false;
}

Activity PlayerBody::GetActivity() const
{
	return ACT_INVALID;
}

bool PlayerBody::IsActivity(Activity a1) const
{
	return false;
}

bool PlayerBody::HasActivityType(unsigned int i1) const
{
	return false;
}

void PlayerBody::SetDesiredPosture(PostureType posture)
{
	this->m_iPosture = posture;
}

PostureType PlayerBody::GetDesiredPosture() const
{
	return this->m_iPosture;
}

bool PlayerBody::IsDesiredPosture(PostureType posture) const
{
	return (posture == this->m_iPosture);
}

bool PlayerBody::IsInDesiredPosture() const
{
	return true;
}

PostureType PlayerBody::GetActualPosture() const
{
	return this->m_iPosture;
}

bool PlayerBody::IsActualPosture(PostureType posture) const
{
	return (posture == this->m_iPosture);
}

bool PlayerBody::IsPostureMobile() const
{
	return true;
}

bool PlayerBody::IsPostureChanging() const
{
	return false;
}

void PlayerBody::SetArousal(ArousalType arousal)
{
	this->m_iArousal = arousal;
}

ArousalType PlayerBody::GetArousal() const
{
	return this->m_iArousal;
}

bool PlayerBody::IsArousal(ArousalType arousal) const
{
	return (arousal == this->m_iArousal);
}

float PlayerBody::GetHullWidth() const
{
	return (VEC_HULL_MAX_SCALED(this->m_Player).x -
		VEC_HULL_MIN_SCALED(this->m_Player).x);
}

float PlayerBody::GetHullHeight() const
{
	if (this->m_iPosture == IBody::PostureType::CROUCH) {
		return this->GetCrouchHullHeight();
	} else {
		return this->GetStandHullHeight();
	}
}

float PlayerBody::GetStandHullHeight() const
{
	return (VEC_HULL_MAX_SCALED(this->m_Player).z -
		VEC_HULL_MIN_SCALED(this->m_Player).z);
}

float PlayerBody::GetCrouchHullHeight() const
{
	/* BUG: should be using the _SCALED versions of these macros! */
	/* report emailed 20151210 */
	return (VEC_DUCK_HULL_MAX.z - VEC_DUCK_HULL_MIN.z);
}

Vector& PlayerBody::GetHullMins() const
{
	if (this->m_iPosture == IBody::PostureType::CROUCH) {
		this->m_vecHullMaxs = VEC_DUCK_HULL_MIN_SCALED(this->m_Player);
	} else {
		this->m_vecHullMaxs = VEC_HULL_MIN_SCALED(this->m_Player);
	}
	
	return this->m_vecHullMins;
}

Vector& PlayerBody::GetHullMaxs() const
{
	if (this->m_iPosture == IBody::PostureType::CROUCH) {
		this->m_vecHullMaxs = VEC_DUCK_HULL_MAX_SCALED(this->m_Player);
	} else {
		this->m_vecHullMaxs = VEC_HULL_MAX_SCALED(this->m_Player);
	}
	
	return this->m_vecHullMaxs;
}

unsigned int PlayerBody::GetSolidMask() const
{
	if (this->m_Player == nullptr) {
		return MASK_PLAYERSOLID;
	}
	
	return this->m_Player->PlayerSolidMask();
}


CBaseEntity *PlayerBody::GetEntity()
{
	return this->m_Player;
}
