/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/NextBotIntentionInterface.cpp
 */


#define FOR_EACH_RESPONDER_QUERY(_func, ...) \
	FOR_EACH_RESPONDER { \
		IContextualQuery *q = dynamic_cast<IContextualQuery *>(responder); \
		if (q != nullptr) { \
			QueryResponse response = q->_func(__VA_ARGS__); \
			if (response != QueryResponse::DONTCARE) { \
				return response; \
			} \
		} \
	} \
	return QueryResponse::DONTCARE;


IIntention::IIntention(INextBot *nextbot)
	: INextBotComponent(nextbot)
{
}

IIntention::~IIntention()
{
}


void IIntention::Reset()
{
	INextBotComponent::Reset();
}

void IIntention::Update()
{
}


QueryResponse IIntention::ShouldPickUp(const INextBot *nextbot, CBaseEntity *it) const
{
	FOR_EACH_RESPONDER_QUERY(ShouldPickUp, nextbot, it);
}

QueryResponse IIntention::ShouldHurry(const INextBot *nextbot) const
{
	FOR_EACH_RESPONDER_QUERY(ShouldHurry, nextbot);
}

QueryResponse IIntention::ShouldRetreat(const INextBot *nextbot) const
{
	FOR_EACH_RESPONDER_QUERY(ShouldRetreat, nextbot);
}

QueryResponse IIntention::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	FOR_EACH_RESPONDER_QUERY(ShouldAttack, nextbot, threat);
}

QueryResponse IIntention::IsHindrance(const INextBot *nextbot, CBaseEntity *it) const
{
	FOR_EACH_RESPONDER_QUERY(IsHindrance, nextbot, it);
}

Vector IIntention::SelectTargetPoint(const INextBot *nextbot, const CBaseCombatCharacter *them) const
{
	FOR_EACH_RESPONDER {
		IContextualQuery *q = dynamic_cast<IContextualQuery *>(responder);
		if (q != nullptr) {
			Vector vec = q->SelectTargetPoint(nextbot, them);
			if (vec != vec3_origin) {
				return vec;
			}
		}
	}
	
	Vector v_world_mins;
	Vector v_world_maxs;
	them->CollisionProp()->WorldSpaceAABB(&v_world_mins, &v_world_maxs);
	
	Vector& v_abs_origin = them->GetAbsOrigin();
	
	float dz = (v_world_maxs.z - v_world_mins.z) * 0.7f;
	
	return {
		.x = v_abs_origin.x,
		.y = v_abs_origin.y,
		.z = v_abs_origin.z + dz,
	};
}

QueryResponse IIntention::IsPositionAllowed(const INextBot *nextbot, const Vector& v1) const
{
	FOR_EACH_RESPONDER_QUERY(IsPositionAllowed, nextbot, v1);
}

const CKnownEntity *IIntention::SelectMoreDangerousThreat(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const
{
	bool obselete1 = (known1 == nullptr || known1->IsObsolete());
	bool obselete2 = (known2 == nullptr || known2->IsObsolete());
	
	if (obsolete1 && obsolete2) {
		return nullptr;
	} else if (obsolete1) {
		return known2;
	} else if (obsolete2) {
		return known1;
	}
	
	FOR_EACH_RESPONDER {
		IContextualQuery *q = dynamic_cast<IContextualQuery *>(responder);
		if (q != nullptr) {
			const CKnownEntity *known = q->SelectMoreDangerousThreat(nextbot, them known1, known2);
			if (known != nullptr) {
				return known;
			}
		}
	}
	
	const Vector *k1_lastknownpos = known1->GetLastKnownPosition();
	float k1_dpos_squared = k1_lastknownpos.DistToSqr(known1->GetAbsOrigin());
	
	const Vector *k2_lastknownpos = known2->GetLastKnownPosition();
	float k2_dpos_squared = k2_lastknownpos.DistToSqr(known2->GetAbsOrigin());
	
	if (k2_dpos_squared > k1_dpos_squared) {
		return known1;
	} else {
		return known2;
	}
}
