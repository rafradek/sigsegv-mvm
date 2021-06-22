/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/NextBotBodyInterface.cpp
 */


IBody::IBody(INextBot *nextbot)
	: INextBotComponent(nextbot);
{
}

IBody::~IBody()
{
}


void IBody::Reset()
{
	INextBotComponent::Reset();
}

void IBody::Update()
{
}


bool IBody::SetPosition(const Vector& pos)
{
	this->GetBot()->GetEntity()->SetAbsOrigin(pos);
	return true;
}

Vector& IBody::GetEyePosition() const
{
	static Vector eye;
	
	eye = this->GetBot()->GetEntity()->WorldSpaceCenter();
	
	return eye;
}

Vector& IBody::GetViewVector() const
{
	static Vector view;
	
	AngleVectors(this->GetBot()->GetEntity()->EyeAngles(), &view);
	
	return view;
}

void IBody::AimHeadTowards(const Vector& vec, LookAtPriorityType priority, float duration, INextBotReply *reply, const char *s1)
{
	if (reply != nullptr) {
		// TODO: enum value for INextBotReply::FailureReason 2
		reply->OnFail(this->GetBot(), 2);
	}
}

void IBody::AimHeadTowards(CBaseEntity *ent, LookAtPriorityType priority, float duration, INextBotReply *reply, const char *s1)
{
	if (reply != nullptr) {
		// TODO: enum value for INextBotReply::FailureReason 2
		reply->OnFail(this->GetBot(), 2);
	}
}

bool IBody::IsHeadAimingOnTarget() const
{
	return false;
}

bool IBody::IsHeadSteady() const
{
	return true;
}

float IBody::GetHeadSteadyDuration() const
{
	return 0.0f;
}

float IBody::GetHeadAimSubjectLeadTime() const
{
	return 0.0f;
}

float IBody::GetHeadAimTrackingInterval() const
{
	return 0.0f;
}

void IBody::ClearPendingAimReply()
{
}

float IBody::GetMaxHeadAngularVelocity() const
{
	return 1000.0f;
}

bool IBody::StartActivity(Activity a1, unsigned int i1)
{
	return false;
}

int IBody::SelectAnimationSequence(Activity a1) const
{
	return 0;
}

Activity IBody::GetActivity() const
{
	return ACT_INVALID;
}

bool IBody::IsActivity(Activity a1) const
{
	return false;
}

bool IBody::HasActivityType(unsigned int i1) const
{
	return false;
}

void IBody::SetDesiredPosture(PostureType posture)
{
}

PostureType IBody::GetDesiredPosture() const
{
	// TODO: need values for IBody::PostureType
	return 0;
}

bool IBody::IsDesiredPosture(PostureType posture) const
{
	return true;
}

bool IBody::IsInDesiredPosture() const
{
	return true;
}

PostureType IBody::GetActualPosture() const
{
	// TODO: need values for IBody::PostureType
	return 0;
}

bool IBody::IsActualPosture(PostureType posture) const
{
	return true;
}

bool IBody::IsPostureMobile() const
{
	return true;
}

bool IBody::IsPostureChanging() const
{
	return false;
}

void IBody::SetArousal(ArousalType arousal)
{
}

ArousalType IBody::GetArousal() const
{
	// TODO: need values for IBody::ArousalType
	return 0;
}

bool IBody::IsArousal(ArousalType arousal) const
{
	return true;
}

float IBody::GetHullWidth() const
{
	return 26.0f;
}

float IBody::GetHullHeight() const
{
	// TODO
}

float IBody::GetStandHullHeight() const
{
	return 68.0f;
}

float IBody::GetCrouchHullHeight() const
{
	return 32.0f;
}

Vector& IBody::GetHullMins() const
{
	static Vector hullMins;
	
	float h_width  = this->GetHullWidth();
	
	hullMins = {
		.x = h_width * -0.5f,
		.y = h_width * -0.5f,
		.z = 0.0f,
	};
	
	return hullMins;
}

Vector& IBody::GetHullMaxs() const
{
	static Vector hullMaxs;
	
	float h_width  = this->GetHullWidth();
	float h_height = this->GetHullHeight();
	
	hullMaxs = {
		.x = h_width * 0.5f,
		.y = h_width * 0.5f,
		.z = h_height,
	};
	
	return hullMaxs;
}

unsigned int IBody::GetSolidMask() const
{
	return MASK_NPCSOLID;
}

int IBody::GetCollisionGroup() const
{
	return COLLISION_GROUP_NONE;
}
