/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/spy/tf_bot_spy_sap.h
 * used in MvM: TODO
 */


// sizeof: 0x4818
class CTFBotSpySap : public Action<CTFBot>
{
public:
	CTFBotSpySap(CBaseObject *target);
	virtual ~CTFBotSpySap();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> OnSuspend(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	
	virtual QueryResponse ShouldRetreat(const INextBot *nextbot) const override;
	virtual QueryResponse ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
	virtual QueryResponse IsHindrance(const INextBot *nextbot, CBaseEntity *it) const override;
	
private:
	bool AreAllDangerousSentriesSapped(CTFBot *actor) const;
	
	CHandle<CBaseObject> m_hTarget;   // +0x0034
	CountdownTimer m_ctRecomputePath; // +0x0038
	PathFollower m_PathFollower;      // +0x0044
};
