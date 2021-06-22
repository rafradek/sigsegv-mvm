/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_destroy_enemy_sentry.h
 * used in MvM: TODO
 */


// sizeof: 0x482c
class CTFBotDestroyEnemySentry : public Action<CTFBot>
{
public:
	CTFBotDestroyEnemySentry();
	virtual ~CTFBotDestroyEnemySentry();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual QueryResponse ShouldHurry(const INextBot *nextbot) const override;
	virtual QueryResponse ShouldRetreat(const INextBot *nextbot) const override;
	virtual QueryResponse ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
	
	static bool IsPossible(CTFBot *actor);
	
private:
	void ComputeCornerAttackSpot(CTFBot *actor);
	void ComputeSafeAttackSpot(CTFBot *actor);
	
	PathFollower m_PathFollower;         // +0x0034
	CountdownTimer m_ctRecomputePath;    // +0x4808
	// 4814 
	Vector m_vecAttackSpot;              // +0x4818
	// 4824 bool, whether we have a valid computed attack spot
	// 4825 bool
	// 4826 bool
	CHandle<CObjectSentrygun> m_hSentry; // +0x4828
};


// sizeof: 0x4818
class CTFBotUberAttackEnemySentry : public Action<CTFBot>
{
public:
	CTFBotUberAttackEnemySentry(CObjectSentrygun *sentry);
	virtual ~CTFBotUberAttackEnemySentry();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual QueryResponse ShouldHurry(const INextBot *nextbot) const override;
	virtual QueryResponse ShouldRetreat(const INextBot *nextbot) const override;
	virtual QueryResponse ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
	
private:
	bool m_bSavedIgnoreEnemies;          // +0x0032
	PathFollower m_PathFollower;         // +0x0034
	CountdownTimer m_ctRecomputePath;    // +0x4808
	CHandle<CObjectSentrygun> m_hSentry; // +0x4814
};


class FindSafeSentryApproachAreaScan : public ISearchSurroundingAreasFunctor
{
public:
	FindSafeSentryApproachAreaScan(/* ??? */);
	virtual ~FindSafeSentryApproachAreaScan();
	
	virtual bool operator()(CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar) override;
	virtual bool ShouldSearch(CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar) override;
	virtual void PostSearch() override;
	
private:
	CTFBot *m_Actor;                  // +0x04
	CUtlVector<CTFNavArea *> m_Areas; // +0x08
	// 1c bool
};


bool FindGrenadeAim(CTFBot *bot, CBaseEntity *target, float *pYaw, float *pPitch);
bool FindStickybombAim(CTFBot *bot, CBaseEntity *target, float *pYaw, float *pPitch, float *f3);
