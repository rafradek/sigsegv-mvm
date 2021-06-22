/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/demoman/tf_bot_prepare_stickybomb_trap.h
 * used in MvM: TODO
 */


// sizeof: 0x58
class CTFBotPrepareStickybombTrap : public Action<CTFBot>
{
public:
	struct BombTargetArea
	{
		CTFNavArea *area;
		int stickies;
	};
	
	CTFBotPrepareStickybombTrap();
	virtual ~CTFBotPrepareStickybombTrap();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> OnSuspend(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnInjured(CTFBot *actor, const CTakeDamageInfo& info) override;
	
	virtual QueryResponse ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
	
	static bool IsPossible(CTFBot *actor);
	
private:
	void InitBombTargetAreas(CTFBot *actor);
	
	bool m_bReload;                               // +0x32
	CTFNavArea *m_LastKnownArea;                  // +0x34
	CUtlVector<BombTargetArea> m_BombTargetAreas; // +0x38
	CountdownTimer m_ctAimTimeout;                // +0x4c
};


class PlaceStickyBombReply : public INextBotReply
{
public:
	PlaceStickyBombReply();
	
	virtual void OnSuccess(INextBot *nextbot) override;
	virtual void OnFail(INextBot *nextbot, FailureReason reason) override;
	
	void Init(BombTargetArea *target, CountdownTimer *timer);
	void Reset();
	
private:
	BombTargetArea *m_pBombTargetArea; // +0x04
	CountdownTimer *m_pctAimTimeout;   // +0x08
};
