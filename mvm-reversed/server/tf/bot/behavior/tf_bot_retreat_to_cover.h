/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_retreat_to_cover.h
 * used in MvM: TODO
 */


// sizeof: 0x482c
class CTFBotRetreatToCover : public Action<CTFBot>
{
public:
	CTFBotRetreatToCover(Action<CTFBot *> *done_action);
	CTFBotRetreatToCover(float duration = -1.0f);
	virtual ~CTFBotRetreatToCover();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual EventDesiredResult<CTFBot> OnMoveToSuccess(CTFBot *actor, const Path *path) override;
	virtual EventDesiredResult<CTFBot> OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail) override;
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	
	virtual QueryResponse ShouldHurry(const INextBot *nextbot) const override;
	
private:
	CTFNavArea *FindCoverArea(CTFBot *actor);
	
	float m_flDuration;                // +0x0034
	Action<CTFBot> *m_DoneAction;      // +0x0038
	PathFollower m_PathFollower;       // +0x003c
	CountdownTimer m_ctRecomputePath;  // +0x4810
	CTFNavArea *m_CoverArea;           // +0x481c
	CountdownTimer m_ctActionDuration; // +0x4820
};


class CSearchForCover : public ISearchSurroundingAreasFunctor
{
public:
	CSearchForCover(/* TODO */);
	virtual ~CSearchForCover();
	
	virtual bool operator()(CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar) override;
	virtual bool ShouldSearch(CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar) override;
	virtual void PostSearch() override;
	
private:
	CTFBot *m_Actor;                  // +0x04
	CUtlVector<CTFNavArea *> m_Areas; // +0x08
	// 1c dword
};


class CTestAreaAgainstThreats : public IVision::IForEachKnownEntity
{
public:
	CTestAreaAgainstThreats(/* TODO */);
	virtual ~CTestAreaAgainstThreats();
	
	virtual bool Inspect(const CKnownEntity& known) override;
	
private:
	CTFBot *m_Actor;  // +0x04
	CNavArea *m_Area; // +0x08
	int m_nVisible;   // +0x0c
};
