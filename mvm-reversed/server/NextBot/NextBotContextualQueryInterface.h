/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/NextBotContextualQueryInterface.h
 */


// TODO: replace this with the three-state enum in the public SDK
enum class QueryResponse : int
{
	// this enum is based entirely on guesswork!
	NO       = 0,
	YES      = 1,
	DONTCARE = 2,
};


class IContextualQuery
{
public:
	virtual ~IContextualQuery();
	
	virtual QueryResponse ShouldPickUp(const INextBot *nextbot, CBaseEntity *it) const;
	virtual QueryResponse ShouldHurry(const INextBot *nextbot) const;
	virtual QueryResponse ShouldRetreat(const INextBot *nextbot) const;
	virtual QueryResponse ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const;
	virtual QueryResponse IsHindrance(const INextBot *nextbot, CBaseEntity *it) const;
	virtual Vector SelectTargetPoint(const INextBot *nextbot, const CBaseCombatCharacter *them) const;
	virtual QueryResponse IsPositionAllowed(const INextBot *nextbot, const Vector& v1) const;
	virtual const CKnownEntity *SelectMoreDangerousThreat(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const;
};
