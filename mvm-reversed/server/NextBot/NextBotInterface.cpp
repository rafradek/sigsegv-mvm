/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/NextBotInterface.cpp
 */


ConVar NextBotDebugHistory("nb_debug_history", "1", FCVAR_CHEAT,
	"If true, each bot keeps a history of debug output in memory");


INextBot::INextBot()
{
	// TODO
}

INextBot::~INextBot()
{
	this->ResetDebugHistory();
	TheNextBots().UnRegister(this);
	
	if (this->m_IntentionInterface != nullptr) {
		delete this->m_IntentionInterface;
	}
	if (this->m_LocoInterface != nullptr) {
		delete this->m_LocoInterface;
	}
	if (this->m_BodyInterface != nullptr) {
		delete this->m_BodyInterface;
	}
	if (this->m_VisionInterface != nullptr) {
		delete this->m_VisionInterface;
	}
}


INextBotEventResponder *INextBot::FirstContainedResponder() const
{
	return this->m_ComponentList;
}

INextBotEventResponder *INextBot::NextContainedResponder(INextBotEventResponder *prev) const
{
	return static_cast<INextBotComponent *>(prev)->m_NextComponent;
}


void INextBot::Reset()
{
	// TODO
}

void INextBot::Update()
{
	// TODO
}

void INextBot::Upkeep()
{
	VPROF_BUDGET("INextBot::Upkeep", "NextBot");
	
	for (INextBotComponent *comp = this->m_ComponentList; comp != nullptr;
		comp = comp->m_NextComponent) {
		comp->Upkeep();
	}
}


bool INextBot::IsRemovedOnReset() const
{
	return true;
}


NextBotCombatCharacter *INextBot::GetNextBotCombatCharacter() const
{
	return nullptr;
}


ILocomotion *INextBot::GetLocomotionInterface() const
{
	if (this->m_LocomotionInterface == nullptr) {
		this->m_LocomotionInterface = new ILocomotion(this);
	}
	
	return this->m_LocomotionInterface;
}

IBody *INextBot::GetBodyInterface() const
{
	if (this->m_BodyInterface == nullptr) {
		this->m_BodyInterface = new IBody(this);
	}
	
	return this->m_BodyInterface;
}

IIntention *INextBot::GetIntentionInterface() const
{
	if (this->m_IntentionInterface == nullptr) {
		this->m_IntentionInterface = new IIntention(this);
	}
	
	return this->m_IntentionInterface;
}

IVision *INextBot::GetVisionInterface() const
{
	if (this->m_VisionInterface == nullptr) {
		this->m_VisionInterface = new IVision(this);
	}
	
	return this->m_VisionInterface;
}


bool INextBot::SetPosition(const Vector& pos)
{
	IBody *body = this->GetBodyInterface();
	if (body != nullptr) {
		return body->SetPosition(pos);
	} else {
		this->GetEntity()->SetAbsOrigin(vec);
		return true;
	}
}

Vector& INextBot::GetPosition() const
{
	return this->GetEntity()->GetAbsOrigin();
}


bool INextBot::IsEnemy(const CBaseEntity *ent) const
{
	if (ent == nullptr) {
		return false;
	}
	
	return (this->GetEntity()->GetTeamNumber != ent->GetTeamNumber());
}

bool INextBot::IsFriend(const CBaseEntity *ent) const
{
	if (ent == nullptr) {
		return false;
	}
	
	return (this->GetEntity()->GetTeamNumber == ent->GetTeamNumber());
}

bool INextBot::IsSelf(const CBaseEntity *ent) const
{
	if (ent == nullptr) {
		return false;
	}
	
	return (ENTINDEX(this->GetEntity()) == ENTINDEX(ent));
}


bool INextBot::IsAbleToClimbOnto(const CBaseEntity *ent) const
{
	if (ent == nullptr || !ent->IsAIWalkable()) {
		return false;
	}
	
	if (ent->ClassMatches("prop_door*") || ent->ClassMatches("func_door*")) {
		return false;
	}
	
	return true;
}

bool INextBot::IsAbleToBreak(const CBaseEntity *ent) const
{
	if (ent == nullptr || ent->m_takedamage != DAMAGE_YES) {
		return false;
	}
	
	if (ent->ClassMatches("func_breakable") && ent->GetHealth() != 0) {
		return true;
	}
	
	if (ent->ClassMatches("func_breakable_surf")) {
		return true;
	}
	
	return (dynamic_cast<CBreakableProp *>(ent) != nullptr);
}

bool INextBot::IsAbleToBlockMovementOf(const INextBot *nextbot) const
{
	return true;
}


bool INextBot::ShouldTouch(const CBaseEntity *ent) const
{
	return true;
}


bool INextBot::IsImmobile() const
{
	return this->m_itImmobileEpoch.HasStarted();
}

float INextBot::GetImmobileDuration() const
{
	return this->m_itImmobileEpoch.GetElapsedTime();
}

void INextBot::ClearImmobileStatus()
{
	this->m_itImmobileEpoch.Invalidate();
	this->m_vecLastPosition = this->GetEntity()->GetAbsOrigin();
}

float INextBot::GetImmobileSpeedThreshold() const
{
	return 30.0f;
}


PathFollower *INextBot::GetCurrentPath() const
{
	return this->m_CurrentPath;
}

void INextBot::SetCurrentPath(const PathFollower *follower)
{
	this->m_CurrentPath = follower;
}

void INextBot::NotifyPathDestruction(const PathFollower *follower)
{
	if (this->m_CurrentPath == follower) {
		this->m_CurrentPath = nullptr;
	}
}


bool INextBot::IsRangeLessThan(CBaseEntity *ent, float dist) const
{
	CBaseEntity *me = this->GetEntity();
	if (me == nullptr || ent == nullptr) {
		return true;
	}
	
	Vector point;
	me->CollisionProp()->CalcNearestPoint(ent->WorldSpaceCenter(), &point);
	return (ent->CollisionProp()->CalcDistanceFromPoint(point) < dist);
}

bool INextBot::IsRangeLessThan(const Vector& vec, float dist) const
{
	return (this->GetPosition() - vec).IsLengthLessThan(dist);
}

bool INextBot::IsRangeGreaterThan(CBaseEntity *ent, float dist) const
{
	CBaseEntity *me = this->GetEntity();
	if (me == nullptr || ent == nullptr) {
		return true;
	}
	
	Vector point;
	me->CollisionProp()->CalcNearestPoint(ent->WorldSpaceCenter(), &point);
	return (ent->CollisionProp()->CalcDistanceFromPoint(point) > dist);
}

bool INextBot::IsRangeGreaterThan(const Vector& vec, float dist) const
{
	return (this->GetPosition() - vec).IsLengthGreaterThan(dist);
}


float INextBot::GetRangeTo(CBaseEntity *ent) const
{
	CBaseEntity *me = this->GetEntity();
	if (me == nullptr || ent == nullptr) {
		return 0.0f;
	}
	
	Vector point;
	me->CollisionProp()->CalcNearestPoint(ent->WorldSpaceCenter(), &point);
	return ent->CollisionProp()->CalcDistanceFromPoint(point);
}

float INextBot::GetRangeTo(const Vector& vec) const
{
	return (this->GetPosition() - vec).Length();
}

float INextBot::GetRangeSquaredTo(CBaseEntity *ent) const
{
	CBaseEntity *me = this->GetEntity();
	if (me == nullptr || ent == nullptr) {
		return 0.0f;
	}
	
	Vector point;
	me->CollisionProp()->CalcNearestPoint(ent->WorldSpaceCenter(), &point);
	return Square(ent->CollisionProp()->CalcDistanceFromPoint(point));
}

float INextBot::GetRangeSquaredTo(const Vector& vec) const
{
	return (this->GetPosition() - vec).LengthSqr();
}


bool INextBot::IsDebugging(unsigned int type) const
{
	if ((TheNextBots().m_nDebugMask & type) != 0) {
		return TheNextBots().IsDebugFilterMatch(this);
	}
	
	return false;
}

char *INextBot::GetDebugIdentifier() const
{
	static char name[0x100];
	V_snprintf(name, sizeof(name), "%s(#%d)",
		this->GetEntity()->GetClassname(), ENTINDEX(this->GetEntity()));
	return name;
}

bool INextBot::IsDebugFilterMatch(const char *filter) const
{
	if (V_strnicmp(filter, this->GetDebugIdentifier(), strlen(filter)) == 0) {
		return true;
	}
	
	CTeam *team = this->GetEntity()->GetTeam();
	if (team != nullptr) {
		return (V_strnicmp(filter, team->GetName(), strlen(filter)) == 0);
	}
	
	return false;
}

void INextBot::DisplayDebugText(const char *text) const
{
	this->GetEntity()->EntityText(this->m_iDebugTextOffset++, text,
		0.1f, RGBA_WHITE);
}


bool INextBot::BeginUpdate()
{
	if (TheNextBots().ShouldUpdate(this)) {
		TheNextBots().NotifyBeginUpdate(this);
		return true;
	}
	
	return false;
}

void INextBot::EndUpdate()
{
	TheNextBots().NotifyEndUpdate(this);
}


void INextBot::DebugConColorMessage(NextBotDebugType type, const Color& color, const char *fmt, ...)
{
	// TODO
}


void INextBot::GetDebugHistory(unsigned int mask, CUtlVector<const NextBotDebugLineType *> *dst) const
{
	// TODO
}

void INextBot::ResetDebugHistory()
{
	// TODO
}


void INextBot::RegisterComponent(INextBotComponent *component)
{
	component->m_NextComponent = this->m_ComponentList;
	this->m_ComponentList = component;
}


void INextBot::UpdateImmobileStatus()
{
	if (!this->m_ctImmobileCheck.IsElapsed()) {
		return;
	}
	
	this->m_ctImmobileCheck.Start(1.0f);
	
	if ((this->GetEntity()->GetAbsOrigin() - this->m_vecLastPosition)
		.IsLengthGreaterThan(this->GetImmobileSpeedThreshold())) {
		this->m_vecLastPosition = this->GetEntity()->GetAbsOrigin();
		this->m_itImmobileEpoch.Invalidate();
	} else if (!this->m_itImmobileEpoch.HasStarted()) {
		this->m_itImmobileEpoch.Start();
	}
}
