/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/NextBotContextualQueryInterface.cpp
 */


// TODO: these should all actually be inline definitions in NextBotContextualQueryInterface.h
// and this file shouldn't exist


QueryResponse IContextualQuery::ShouldPickUp(const INextBot *nextbot, CBaseEntity *ent) const
{
	return QueryResponse::DONTCARE;
}

QueryResponse IContextualQuery::ShouldHurry(const INextBot *nextbot) const
{
	return QueryResponse::DONTCARE;
}

QueryResponse IContextualQuery::ShouldRetreat(const INextBot *nextbot) const
{
	return QueryResponse::DONTCARE;
}

QueryResponse IContextualQuery::ShouldAttack(const INextBot *nextbot, const CKnownEntity *known) const
{
	return QueryResponse::DONTCARE;
}

QueryResponse IContextualQuery::IsHindrance(const INextBot *nextbot, CBaseEntity *ent) const
{
	return QueryResponse::DONTCARE;
}

Vector IContextualQuery::SelectTargetPoint(const INextBot *nextbot, const CBaseCombatCharacter *who) const
{
	return vec3_origin;
}

QueryResponse IContextualQuery::IsPositionAllowed(const INextBot *nextbot, const Vector& v1) const
{
	return QueryResponse::DONTCARE;
}

const CKnownEntity *IContextualQuery::SelectMoreDangerousThreat(const INextBot *nextbot, const CBaseCombatCharacter *who, const CKnownEntity *known1, const CKnownEntity *known2) const
{
	return nullptr;
}
