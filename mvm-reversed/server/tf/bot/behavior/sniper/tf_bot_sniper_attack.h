/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/sniper/tf_bot_sniper_attack.h
 * used in MvM: TODO
 */


// sizeof: 0x40+
class CTFBotSniperAttack : public Action<CTFBot>
{
public:
	CTFBotSniperAttack(/* TODO */);
	virtual ~CTFBotSniperAttack();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> OnSuspend(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual Vector SelectTargetPoint(const INextBot *nextbot, const CBaseCombatCharacter *them) const override;
	virtual const CKnownEntity *SelectMoreDangerousThreat(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const override;
	
	static bool IsPossible(CTFBot *actor);
	
private:
	bool IsImmediateThreat(const CBaseCombatCharacter *who, const CKnownEntity *threat) const;
	
	CountdownTimer m_ctLinger; // +0x34
};
