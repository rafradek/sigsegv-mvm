/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/Path/NextBotPathFollow.cpp
 */


ConVar NextBotSpeedLookAheadRange("nb_speed_look_ahead_range", "150", FCVAR_CHEAT);
ConVar NextBotGoalLookAheadRange("nb_goal_look_ahead_range", "50", FCVAR_CHEAT);
ConVar NextBotLadderAlignRange("nb_ladder_align_range", "50", FCVAR_CHEAT);
ConVar NextBotAllowAvoiding("nb_allow_avoiding", "1", FCVAR_CHEAT);
ConVar NextBotAllowClimbing("nb_allow_climbing", "1", FCVAR_CHEAT);
ConVar NextBotAllowGapJumping("nb_allow_gap_jumping", "1", FCVAR_CHEAT);
ConVar NextBotDebugClimbing("nb_debug_climbing", "0", FCVAR_CHEAT);


PathFollower::PathFollower()
{
	this->m_CurrentGoal = nullptr;
	// byte @ 0x477c = 0x00
	this->m_flMinLookAheadDistance = -1.0f;
	// float @ 0x47d0 = 25.0f
}

PathFollower::~PathFollower()
{
	// TODO
	
	// INextBot::NotifyPathDestruction(this)
}


const Segment *PathFollower::GetCurrentGoal() const
{
	return this->m_CurrentGoal;
}


void PathFollower::Invalidate()
{
	// TODO
}


void PathFollower::Draw(const Segment *seg) const
{
	// TODO
}


void PathFollower::OnPathChanged(INextBot *nextbot, ResultType rtype)
{
	this->m_CurrentGoal = this->FirstSegment();
}


void PathFollower::Update(INextBot *nextbot)
{
	VPROF_BUDGET("PathFollower::Update", "NextBotSpiky");
	
	// TODO
}

void PathFollower::SetMinLookAheadDistance(float dist)
{
	this->m_flMinLookAheadDistance = dist;
}

CBaseEntity *PathFollower::GetHindrance() const
{
	return this->m_hHindrance;
}

bool PathFollower::IsDiscontinuityAhead(INextBot *nextbot, SegmentType stype, float f1) const
{
	// TODO
}


void PathFollower::AdjustSpeed(INextBot *nextbot)
{
	// TODO
}

Vector PathFollower::Avoid(PathFollower::INextBot *nextbot, const Vector& v1, const Vector& v2, const Vector& v3)
{
	VPROF_BUDGET("PathFollower::Avoid", "NextBotExpensive");
	
	// TODO
}

bool PathFollower::CheckProgress(INextBot *nextbot)
{
	// TODO
}

bool PathFollower::Climbing(INextBot *nextbot, const Segment *seg, const Vector& v1, const Vector& v2, float f1)
{
	VPROF_BUDGET("PathFollower::Climbing", "NextBot");
	
	// TODO
}

CBaseEntity *PathFollower::FindBlocker(INextBot *nextbot)
{
	// TODO
}

bool PathFollower::IsAtGoal(INextBot *nextbot) const
{
	VPROF_BUDGET("PathFollower::IsAtGoal", "NextBot");
	
	// TODO
}

bool PathFollower::JumpOverGaps(INextBot *nextbot, const Segment *seg, const Vector& v1, const Vector& v2, float f1)
{
	VPROF_BUDGET("PathFollower::JumpOverGaps", "NextBot");
	
	// TODO
}

bool PathFollower::LadderUpdate(INextBot *nextbot)
{
	VPROF_BUDGET("PathFollower::LadderUpdate", "NextBot");
	
	// TODO
}

