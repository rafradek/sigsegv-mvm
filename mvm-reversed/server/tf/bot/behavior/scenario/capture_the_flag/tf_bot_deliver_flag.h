/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/scenario/capture_the_flag/tf_bot_deliver_flag.h
 * used in MvM: TODO
 */


// sizeof: 0x4834
class CTFBotDeliverFlag : public Action<CTFBot>
{
public:
	CTFBotDeliverFlag();
	virtual ~CTFBotDeliverFlag();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnContact(CTFBot *actor, CBaseEntity *ent, CGameTrace *trace) override;
	
	virtual QueryResponse ShouldHurry(const INextBot *nextbot) const override;
	virtual QueryResponse ShouldRetreat(const INextBot *nextbot) const override;
	virtual QueryResponse ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
	
private:
	bool UpgradeOverTime(CTFBot *actor);
	
	PathFollower m_PathFollower;      // +0x0034
	CountdownTimer m_ctRecomputePath; // +0x4808
	// 4814 float
	// 4818 CountdownTimer
	// 4824 int
	// 4828 CountdownTimer
};


// sizeof: 0x4818
class CTFBotPushToCapturePoint : public Action<CTFBot>
{
public:
	CTFBotPushToCapturePoint(Action<CTFBot> *done_action);
	virtual ~CTFBotPushToCapturePoint();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual EventDesiredResult<CTFBot> OnNavAreaChanged(CTFBot *actor, CNavArea *area1, CNavArea *area2) override;
	
private:
	PathFollower m_PathFollower;      // +0x0034
	CountdownTimer m_ctRecomputePath; // +0x4808
	Action<CTFBot> *m_DoneAction;     // +0x4814
};
