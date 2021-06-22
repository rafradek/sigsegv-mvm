/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_behavior.h
 * used in MvM: TODO
 */


// sizeof: 0x74
class CTFBotMainAction : public Action<CTFBot>
{
public:
	CTFBotMainAction(/* TODO */);
	virtual ~CTFBotMainAction();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual Action<CTFBot> *InitialContainedAction(CTFBot *actor) override;
	
	virtual EventDesiredResult<CTFBot> OnContact(CTFBot *actor, CBaseEntity *ent, CGameTrace *trace) override;
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	virtual EventDesiredResult<CTFBot> OnInjured(CTFBot *actor, const CTakeDamageInfo& info) override;
	virtual EventDesiredResult<CTFBot> OnOtherKilled(CTFBot *actor, CBaseCombatCharacter *who, const CTakeDamageInfo& info) override;
	
	virtual QueryResponse ShouldHurry(const INextBot *nextbot) const override;
	virtual QueryResponse ShouldRetreat(const INextBot *nextbot) const override;
	virtual QueryResponse ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
	virtual Vector SelectTargetPoint(const INextBot *nextbot, const CBaseCombatCharacter *them) const override;
	virtual QueryResponse IsPositionAllowed(const INextBot *nextbot, const Vector& v1) const override;
	virtual const CKnownEntity *SelectMoreDangerousThreat(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const override;
	
private:
	void Dodge(CTFBot *actor);
	void FireWeaponAtEnemy(CTFBot *actor);
	const CKnownEntity *GetHealerOfThreat(const CKnownEntity *threat) const;
	bool IsImmediateThreat(const CBaseCombatCharacter *who, const CKnownEntity *threat) const;
	const CKnownEntity *SelectCloserThreat(CTFBot *actor, const CKnownEntity *threat1, const CKnownEntity *threat2) const;
	const CKnownEntity *SelectMoreDangerousThreatInternal(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const;
	
	// TODO 34
	// TODO 38
	// TODO 3c
	// TODO 40
	// TODO 44
	// TODO 48
	// TODO 4c dword 0
	// TODO 50 dword 0
	// TODO 54 dword 0
	// TODO 58 dword 0
	// TODO 5c IntervalTimer
	// TODO 60 dword 0, related to disguising
	bool m_bReloadingBarrage; // +0x64
	// TODO 68 CHandle<CBaseEntity>, set in OnContact to the entity touching us
	// TODO 6c float, set in OnContact to the time when we touched a solid non-player entity
	IntervalTimer m_itUnderground; // +0x70
};


const CKnownEntity *SelectClosestSpyToMe(CTFBot *actor, const CKnownEntity *known1, const CKnownEntity *known2);


class CCompareFriendFoeInfluence : public IVision::IForEachKnownEntity
{
	// TODO
};
