/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_move_to_vantage_point.h
 * used in MvM: TODO
 */


// sizeof: 0x481c
class CTFBotMoveToVantagePoint : public Action<CTFBot>
{
public:
	CTFBotMoveToVantagePoint(float max_cost);
	virtual ~CTFBotMoveToVantagePoint();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual EventDesiredResult<CTFBot> OnMoveToSuccess(CTFBot *actor, const Path *path) override;
	virtual EventDesiredResult<CTFBot> OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail) override;
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	
private:
	float m_flMaxCost;                // +0x0034
	PathFollower m_PathFollower;      // +0x0038
	CountdownTimer m_ctRecomputePath; // +0x480c
	CTFNavArea *m_VantagePoint;       // +0x4818
};
