/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/engineer/tf_bot_engineer_move_to_build.h
 * used in MvM: TODO
 */


// sizeof: 0x4848
class CTFBotEngineerMoveToBuild : public Action<CTFBot>
{
public:
	CTFBotEngineerMoveToBuild();
	virtual ~CTFBotEngineerMoveToBuild();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual EventDesiredResult<CTFBot> OnMoveToSuccess(CTFBot *actor, const Path *path) override;
	virtual EventDesiredResult<CTFBot> OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail) override;
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	virtual EventDesiredResult<CTFBot> OnTerritoryLost(CTFBot *actor, int i1) override;
	
private:
	void CollectBuildAreas(CTFBot *actor);
	void SelectBuildLocation(CTFBot *actor);
	
	// 0034 CHandle<T>
	// 0038 Vector
	PathFollower m_PathFollower;           // +0x0044
	// 4818 CountdownTimer
	CUtlVector<CTFNavArea *> m_BuildAreas; // +0x4824
	// 4838 
	// 483c CountdownTimer
};


bool CompareRangeToPoint(CTFNavArea *const *area1, CTFNavArea *const *area2);
