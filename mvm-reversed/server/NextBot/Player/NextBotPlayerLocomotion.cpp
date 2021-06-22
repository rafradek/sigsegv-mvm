/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/Player/NextBotPlayerLocomotion.cpp
 */


ConVar NextBotPlayerMoveDirect("nb_player_move_direct", "0", FCVAR_NONE);


PlayerLocomotion::PlayerLocomotion(INextBot *nextbot)
	: ILocomotion(nextbot)
{
	this->m_Player = nullptr;
	
	this->Reset();
}

PlayerLocomotion::~PlayerLocomotion()
{
}


void PlayerLocomotion::Reset()
{
	this->m_Player = static_cast<CBasePlayer *>(this->GetBot()->GetEntity());
	
	this->m_bJumping    = false;
	this->m_bClimbing   = false;
	this->m_bGapJumping = false;
	// TODO: bool @ +0x7c = false
	
	this->m_flDesiredSpeed = 0.0f;
	
	this->m_iLadderState = LadderState::NONE;
	this->m_NavLadder = nullptr;
	this->m_NavArea   = nullptr;
	
	this->m_ctPlayerLoco2.Invalidate();
	
	this->m_flMinSpeedLimit = 0.0f;
	this->m_flMaxSpeedLimit = 1.0e+7f;
	
	ILocomotion::Reset();
}

void PlayerLocomotion::Update()
{
	if (!this->TraverseLadder() && (this->m_bGapJumping || this->m_bClimbing)) {
		this->SetMinimumSpeedLimit(this->GetRunSpeed());
		
		// TODO
		
		this->Approach(this->m_vecClimbJump, 1.0f);
	}
	
	ILocomotion::Update();
}


void PlayerLocomotion::Approach(const Vector& dst, float f1)
{
	VPROF_BUDGET("PlayerLocomotion::Approach", "NextBot");
	
	ILocomotion::Approach(dst, 1.0f);
	this->AdjustPosture(dst);
	
	if (this->GetBot()->IsDebugging(DEBUG_LOCOMOTION)) {
		NDebugOverlay::Line(this->GetFeet(), dst, RGB_YELLOW, true, 0.1f);
	}
	
	INextBot *nextbot = this->GetBot();
	INextBotPlayerInput *input;
	if (nextbot == nullptr || (input =
		dynamic_cast<INextBotPlayerInput *>(nextbot)) == nullptr) {
		DevMsg("PlayerLocomotion::Approach: No INextBotPlayerInput\n ");
		return;
	}
	
	Vector eye_vec;
	this->m_Player->EyeVectors(&eye_vec);
	
	Vector2D& eye_xy = eye_vec.AsVector2D();
	eye_xy.NormalizeInPlace();
	
	Vector2D eye_xy_90 = Vector2D(eye_xy.y, -eye_xy.x);
	
	Vector2D feet_to_dst = dst.AsVector2D() - this->GetFeet().AsVector2D();
	feet_to_dst.NormalizeInPlace();
	
	float forward = feet_to_dst.Dot(eye_xy);
	float side = feet_to_dst.Dot(eye_xy_90);
	
	if (this->m_Player->IsOnLadder() && this->IsUsingLadder() &&
		(this->m_iLadderState == LadderState::ASCEND ||
		this->m_iLadderState == LadderState::DESCEND)) {
		// TODO: ladder stuff
	} else {
		if (NextBotPlayerMoveDirect.GetBool() && feet_to_dst.Length() > 0.25f) {
			input->SetButtonScale(forward, side);
		}
		
		if (forward > 0.25f) {
			input->PressForwardButton();
			
			if (this->GetBot()->IsDebugging(DEBUG_LOCOMOTION)) {
				NDebugOverlay::HorzArrow(this->m_Player->GetAbsOrigin(),
					this->m_Player->GetAbsOrigin() +
					Vector(eye_xy.x * 50.0f, eye_xy.y * 50.0f, 0.0f),
					15.0f, RGBA_GREEN, true, 0.1f);
			}
		} else if (forward < -0.25f) {
			input->PressBackwardButton();
			
			if (this->GetBot()->IsDebugging(DEBUG_LOCOMOTION)) {
				NDebugOverlay::HorzArrow(this->m_Player->GetAbsOrigin(),
					this->m_Player->GetAbsOrigin() -
					Vector(eye_xy.x * 50.0f, eye_xy.y * 50.0f, 0.0f),
					15.0f, RGBA_RED, true, 0.1f);
			}
		}
		
		if (side <= -0.25f) {
			input->PressLeftButton();
			
			if (this->GetBot()->IsDebugging(DEBUG_LOCOMOTION)) {
				NDebugOverlay::HorzArrow(this->m_Player->GetAbsOrigin(),
					this->m_Player->GetAbsOrigin() -
					Vector(eye_xy_90.x * 50.0f, eye_xy_90.y * 50.0f, 0.0f),
					15.0f, RGBA_MAGENTA, true, 0.1f);
			}
		} else if (size >= 0.25f) {
			input->PressRightButton();
			
			if (this->GetBot()->IsDebugging(DEBUG_LOCOMOTION)) {
				NDebugOverlay::HorzArrow(this->m_Player->GetAbsOrigin(),
					this->m_Player->GetAbsOrigin() +
					Vector(eye_xy_90.x * 50.0f, eye_xy_90.y * 50.0f, 0.0f),
					15.0f, RGBA_CYAN, true, 0.1f);
			}
		}
	}
	
	if (!this->IsRunning()) {
		input->PressWalkButton();
	}
}

void PlayerLocomotion::DriveTo(const Vector& vec)
{
	ILocomotion::DriveTo(vec);
	this->Approach(vec, 1.0f);
}

bool PlayerLocomotion::ClimbUpToLedge(const Vector& v1, const Vector& v2, const CBaseEntity *ent)
{
	if (!this->IsClimbPossible(this->GetBot(), ent)) {
		return;
	}
	
	this->Jump();
	this->m_bClimbing = true;
	// TODO: bool @ +0x7c = false
	this->m_vecClimbJump = v1;
}

void PlayerLocomotion::JumpAcrossGap(const Vector& v1, const Vector& v2)
{
	this->Jump();
	this->GetBot()->GetBodyInterface()->AimHeadTowards(v1,
		IBody::LookAtPriorityType::OVERRIDE_ALL, 1.0f, nullptr,
		"Looking forward while jumping a gap");
	this->m_bGapJumping = true;
	// TODO: bool @ +0x7c = false
	this->m_vecClimbJump = v1;
}

void PlayerLocomotion::Jump()
{
	this->m_bJumping = true;
	
	this->m_ctJump.Start(0.5f);
	
	INextBotPlayerInput *input =
		dynamic_cast<INextBotPlayerInput *>(this->GetBot());
	if (input != nullptr) {
		input->PressJumpButton();
	}
}

bool PlayerLocomotion::IsClimbingOrJumping() const
{
	if (!this->m_bJumping) {
		return false;
	}
	
	if (this->m_ctJump.IsElapsed() && this->IsOnGround()) {
		this->m_bJumping = false;
		return false;
	}
	
	return true;
}

bool PlayerLocomotion::IsClimbingUpToLedge() const
{
	return this->m_bClimbing;
}

bool PlayerLocomotion::IsJumpingAcrossGap() const
{
	return this->m_bGapJumping;
}

void PlayerLocomotion::Run()
{
	this->m_flDesiredSpeed = this->GetRunSpeed();
}

void PlayerLocomotion::Walk()
{
	this->m_flDesiredSpeed = this->GetWalkSpeed();
}

void PlayerLocomotion::Stop()
{
	this->m_flDesiredSpeed = 0.0f;
}

bool PlayerLocomotion::IsRunning() const
{
	return true;
}

void PlayerLocomotion::SetDesiredSpeed(float speed)
{
	this->m_flDesiredSpeed = speed;
}

float PlayerLocomotion::GetDesiredSpeed() const
{
	return Clamp(this->m_flDesiredSpeed,
		this->m_flMinSpeedLimit, this->m_flMaxSpeedLimit);
}

bool PlayerLocomotion::IsOnGround() const
{
	return (this->m_Player->GetGroundEntity() != nullptr);
}

CBaseEntity *PlayerLocomotion::GetGround() const
{
	return this->m_Player->GetGroundEntity();
}

Vector& PlayerLocomotion::GetGroundNormal() const
{
	static Vector up(0.0f, 0.0f, 1.0f);
	return up;
}

void PlayerLocomotion::ClimbLadder(const CNavLadder *ladder, const CNavArea *area)
{
	this->m_iLadderState = LadderState::ASCEND_APPROACH;
	
	this->m_NavLadder = ladder;
	this->m_NavArea   = area;
}

void PlayerLocomotion::DescendLadder(const CNavLadder *ladder, const CNavArea *area)
{
	this->m_iLadderState = LadderState::DESCEND_APPROACH;
	
	this->m_NavLadder = ladder;
	this->m_NavArea   = area;
}

bool PlayerLocomotion::IsUsingLadder() const
{
	return (this->m_iLadderState != LadderState::NONE);
}

bool PlayerLocomotion::IsAscendingOrDescendingLadder() const
{
	switch (this->m_iLadderState) {
	
	case LadderState::ASCEND:
	case LadderState::DESCEND:
	case LadderState::ASCEND_DISMOUNT:
	case LadderState::DESCEND_DISMOUNT:
		return true;
		
	case LadderState::ASCEND_APPROACH:
	case LadderState::DESCEND_APPROACH:
	case LadderState::NONE:
	default:
		return false;
	}
}

bool PlayerLocomotion::IsAbleToAutoCenterOnLadder() const
{
	if (!this->IsUsingLadder()) {
		return false;
	}
	
	return (this->m_iLadderState == LadderState::ASCEND ||
		this->m_iLadderState == LadderState::DESCEND);
}

void PlayerLocomotion::FaceTowards(const Vector& vec)
{
	Vector look_at(vec.x, vec.y, this->GetBot()->GetEntity()->EyePosition().z);
	this->GetBot()->GetBodyInterface()->AimHeadTowards(look_at,
		IBody::LookAtPriorityType::BORING, 0.1f, nullptr, "Body facing");
}

void PlayerLocomotion::SetDesiredLean(const QAngle& ang)
{
}

QAngle& PlayerLocomotion::GetDesiredLean() const
{
	static QAngle junk;
	return junk;
}

Vector& PlayerLocomotion::GetFeet() const
{
	return this->m_Player->GetAbsOrigin();
}

float PlayerLocomotion::GetStepHeight() const
{
	return 18.0f;
}

float PlayerLocomotion::GetMaxJumpHeight() const
{
	return 57.0f;
}

float PlayerLocomotion::GetDeathDropHeight() const
{
	return 200.0f;
}

float PlayerLocomotion::GetRunSpeed() const
{
	return this->m_Player->MaxSpeed();
}

float PlayerLocomotion::GetWalkSpeed() const
{
	return (0.5f * this->m_Player->MaxSpeed());
}

float PlayerLocomotion::GetMaxAcceleration() const
{
	return 100.0f;
}

float PlayerLocomotion::GetMaxDeceleration() const
{
	return 200.0f;
}

Vector& PlayerLocomotion::GetVelocity() const
{
	return this->m_Player->GetAbsVelocity();
}

void PlayerLocomotion::AdjustPosture(const Vector& vec)
{
	PlayerBody *body = this->GetBot()->GetBodyInterface();
	
	// TODO: enum values for IBody::PostureType
	if (body->IsActualPosture((IBody::PostureType)0) ||
		body->IsActualPosture((IBody::PostureType)1)) {
		ILocomotion::AdjuctPosture(vec);
	}
}


void PlayerLocomotion::SetMinimumSpeedLimit(float speed)
{
	this->m_flMinSpeedLimit = speed;
}

void PlayerLocomotion::SetMaximumSpeedLimit(float speed)
{
	this->m_flMaxSpeedLimit = speed;
}


bool PlayerLocomotion::IsClimbPossible(INextBot *nextbot, const CBaseEntity *ent)
{
	// TODO
}


bool PlayerLocomotion::TraverseLadder()
{
	switch (this->m_iLadderState) {
		
	case LadderState::ASCEND_APPROACH:
		this->m_iLadderState = this->ApproachAscendingLadder();
		return true;
		
	case LadderState::DESCEND_APPROACH:
		this->m_iLadderState = this->ApproachDescendingLadder();
		return true;
		
	case LadderState::ASCEND:
		this->m_iLadderState = this->AscendLadder();
		return true;
		
	case LadderState::DESCEND:
		this->m_iLadderState = this->DescendLadder();
		return true;
		
	case LadderState::ASCEND_DISMOUNT:
		this->m_iLadderState = this->DismountLadderTop();
		return true;
		
	case LadderState::DESCEND_DISMOUNT:
		this->m_iLadderState = this->DismountLadderBottom();
		return true;
		
	default:
		this->m_NavLadder = nullptr;
		if (this->GetBot()->GetEntity()->GetMoveType() == MOVETYPE_LADDER) {
			this->GetBot()->GetEntity()->SetMoveType(MOVETYPE_WALK);
		}
		
		return false;
	}
}

PlayerLocomotion::LadderState PlayerLocomotion::ApproachAscendingLadder()
{
	// TODO
}

PlayerLocomotion::LadderState PlayerLocomotion::ApproachDescendingLadder()
{
	// TODO
}

PlayerLocomotion::LadderState PlayerLocomotion::AscendLadder()
{
	// TODO
}

PlayerLocomotion::LadderState PlayerLocomotion::DescendLadder()
{
	// TODO
}

PlayerLocomotion::LadderState PlayerLocomotion::DismountLadderTop()
{
	// TODO
}

PlayerLocomotion::LadderState PlayerLocomotion::DismountLadderBottom()
{
	// TODO
}
