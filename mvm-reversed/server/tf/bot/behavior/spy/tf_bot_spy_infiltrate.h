/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/spy/tf_bot_spy_infiltrate.h
 * used in MvM: TODO
 */


// sizeof: 0x4834
class CTFBotSpyInfiltrate : public Action<CTFBot>
{
public:
	CTFBotSpyInfiltrate();
	virtual ~CTFBotSpyInfiltrate();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> OnSuspend(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	virtual EventDesiredResult<CTFBot> OnTerritoryCaptured(CTFBot *actor, int i1) override;
	virtual EventDesiredResult<CTFBot> OnTerritoryLost(CTFBot *actor, int i1) override;
	
	virtual QueryResponse ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
	
private:
	bool FindHidingSpot(CTFBot *actor);
	
	CountdownTimer m_ctRecomputePath;  // +0x0034
	PathFollower m_PathFollower;       // +0x0040
	CTFNavArea *m_HidingArea;          // +0x4814
	CountdownTimer m_ctFindHidingArea; // +0x4818
	CountdownTimer m_ctWait;           // +0x4824
	bool m_bCloaked;                   // +0x4830
};
