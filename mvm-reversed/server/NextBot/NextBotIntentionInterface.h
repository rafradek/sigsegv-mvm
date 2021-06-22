/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/NextBotIntentionInterface.h
 */


class IIntention : public INextBotComponent, public IContextualQuery
{
public:
	IIntention(INextBot *nextbot);
	virtual ~IIntention();
	
	virtual void Reset() override;
	virtual void Update() override;
	
	virtual QueryResponse ShouldPickUp(const INextBot *nextbot, CBaseEntity *it) const override;
	virtual QueryResponse ShouldHurry(const INextBot *nextbot) const override;
	virtual QueryResponse ShouldRetreat(const INextBot *nextbot) const override;
	virtual QueryResponse ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
	virtual QueryResponse IsHindrance(const INextBot *nextbot, CBaseEntity *it) const override;
	virtual Vector SelectTargetPoint(const INextBot *nextbot, const CBaseCombatCharacter *them) const override;
	virtual QueryResponse IsPositionAllowed(const INextBot *nextbot, const Vector& v1) const override;
	virtual const CKnownEntity *SelectMoreDangerousThreat(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const override;
};
