/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/scenario/payload/tf_bot_payload_guard.h
 * used in MvM: TODO
 */


// sizeof: 0x4838
class CTFBotPayloadGuard : public Action<CTFBot>
{
public:
	CTFBotPayloadGuard();
	virtual ~CTFBotPayloadGuard();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnMoveToSuccess(CTFBot *actor, const Path *path) override;
	virtual EventDesiredResult<CTFBot> OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail) override;
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	virtual EventDesiredResult<CTFBot> OnTerritoryContested(CTFBot *actor, int i1) override;
	virtual EventDesiredResult<CTFBot> OnTerritoryCaptured(CTFBot *actor, int i1) override;
	virtual EventDesiredResult<CTFBot> OnTerritoryLost(CTFBot *actor, int i1) override;
	
	virtual QueryResponse ShouldHurry(const INextBot *nextbot) const override;
	virtual QueryResponse ShouldRetreat(const INextBot *nextbot) const override;
	
private:
	Vector FindVantagePoint(CTFBot *actor, CBaseEntity *target);
	
	PathFollower m_PathFollower;      // +0x0034
	CountdownTimer m_ctRecomputePath; // +0x4808
	// 4814 Vector
	// 4820 CountdownTimer
	// 482c CountdownTimer
};


class CCollectPayloadGuardVantagePoints : public ISearchSurroundingAreasFunctor
{
public:
	CCollectPayloadGuardVantagePoints(CTFBot *actor, CBaseEntity *target) :
		m_Actor(actor), m_Target(target) {}
	
	virtual bool operator()(CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar) override;
	
	Vector GetResult() const;
	
private:
	CTFBot *m_Actor;                    // +0x04
	CBaseEntity *m_Target;              // +0x08
	CUtlVector<Vector> m_VantagePoints; // +0x0c
};
