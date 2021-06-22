/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_seek_and_destroy.h
 * used in MvM: TODO
 */


// sizeof: 0x4828
class CTFBotSeekAndDestroy : public Action<CTFBot>
{
public:
	CTFBotSeekAndDestroy(float duration = -1.0f);
	virtual ~CTFBotSeekAndDestroy();
	
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
	CTFNavArea *ChooseGoalArea(CTFBot *actor);
	void RecomputeSeekPath(CTFBot *actor);
	
	CTFNavArea *m_GoalArea;            // +0x0034
	bool m_bPointLocked;               // +0x0038
	PathFollower m_PathFollower;       // +0x003c
	CountdownTimer m_ctRecomputePath;  // +0x4810
	CountdownTimer m_ctActionDuration; // +0x481c
};
