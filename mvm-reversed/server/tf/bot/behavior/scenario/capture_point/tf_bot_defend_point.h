/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/scenario/capture_point/tf_bot_defend_point.h
 * used in MvM: TODO
 */


// sizeof: 0x9034
class CTFBotDefendPoint : public Action<CTFBot>
{
public:
	CTFBotDefendPoint();
	virtual ~CTFBotDefendPoint();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnContact(CTFBot *actor, CBaseEntity *ent, CGameTrace *trace) override;
	virtual EventDesiredResult<CTFBot> OnMoveToSuccess(CTFBot *actor, const Path *path) override;
	virtual EventDesiredResult<CTFBot> OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail) override;
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	virtual EventDesiredResult<CTFBot> OnTerritoryContested(CTFBot *actor, int i1) override;
	virtual EventDesiredResult<CTFBot> OnTerritoryCaptured(CTFBot *actor, int i1) override;
	virtual EventDesiredResult<CTFBot> OnTerritoryLost(CTFBot *actor, int i1) override;
	
private:
	bool IsPointThreatened(CTFBot *actor);
	CTFNavArea *SelectAreaToDefendFrom(CTFBot *actor);
	bool WillBlockCapture(CTFBot *actor) const;
	
	PathFollower m_PathFollower; // +0x0034
	ChasePath m_ChasePath;       // +0x4808
	// 9008 CountdownTimer
	// 9014 CountdownTimer
	// 9020 CountdownTimer
	CTFNavArea *m_DefenseArea;   // +0x902c
	bool m_bShouldRoam;          // +0x9030
};


class CSelectDefenseAreaForPoint : public ISearchSurroundingAreasFunctor
{
public:
	CSelectDefenseAreaForPoint(/* TODO */);
	
	virtual bool operator()(CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar) override;
	virtual bool ShouldSearch(CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar) override;
	
private:
	// 04 ptr to CTeamControlPoint? I think?
	// 08 CUtlVector<CTFNavArea *> * results (where the hell is this alloc'd or free'd?)
	// 0c float
	int m_iTeamNum; // +0x10
};
