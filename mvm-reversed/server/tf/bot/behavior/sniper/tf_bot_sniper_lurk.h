/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/sniper/tf_bot_sniper_lurk.h
 * used in MvM: TODO
 */


// sizeof: 0x485c
class CTFBotSniperLurk : public Action<CTFBot>
{
public:
	CTFBotSniperLurk();
	virtual ~CTFBotSniperLurk();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> OnSuspend(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual QueryResponse ShouldRetreat(const INextBot *nextbot) const override;
	virtual QueryResponse ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
	virtual const CKnownEntity *SelectMoreDangerousThreat(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const override;
	
private:
	CTFBotHint *FindHint(CTFBot *actor);
	bool FindNewHome(CTFBot *actor);
	
	CountdownTimer m_ctPatience;             // +0x0034
	CountdownTimer m_ctRecomputePath;        // +0x0040
	PathFollower m_PathFollower;             // +0x004c
	// 4820 int
	Vector m_vecHome;                        // +0x4824
	bool m_bHasHome;                         // +0x4830
	bool m_bNearHome;                        // +0x4831
	// 4834 CountdownTimer
	bool m_bOpportunistic;                   // +0x4840
	CUtlVector<CHandle<CTFBotHint>> m_Hints; // +0x4844
	CHandle<CTFBotHint> m_hHint;             // +0x4858
};
