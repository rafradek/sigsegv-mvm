/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/scenario/capture_point/tf_bot_defend_point_block_capture.h
 * used in MvM: TODO
 */


// sizeof: 0x481c
class CTFBotDefendPointBlockCapture : public Action<CTFBot>
{
public:
	CTFBotDefendPointBlockCapture();
	virtual ~CTFBotDefendPointBlockCapture();
	
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
	bool IsPointSafe(CTFBot *actor);
	
	PathFollower m_PathFollower;      // +0x0034
	CountdownTimer m_ctRecomputePath; // +0x4808
};
