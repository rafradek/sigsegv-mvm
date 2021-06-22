/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_get_health.h
 * used in MvM: TODO
 */


// sizeof: 0x4810
class CTFBotGetHealth : public Action<CTFBot>
{
public:
	CTFBotGetHealth();
	virtual ~CTFBotGetHealth();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual EventDesiredResult<CTFBot> OnMoveToSuccess(CTFBot *actor, const Path *path) override;
	virtual EventDesiredResult<CTFBot> OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail) override;
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	
	virtual QueryResponse ShouldHurry(const INextBot *nextbot) const override;
	
	static bool IsPossible(CTFBot *actor);
	
private:
	PathFollower m_PathFollower;    // +0x0034
	CHandle<CBaseEntity> m_hHealth; // +0x4808
	// 4808 CHandle<T>
	// 480c 
};


class CHealthFilter : public INextBotFilter
{
public:
	CHealthFilter(CBaseEntity *actor);
	
	virtual bool IsSelected(const CBaseEntity *ent) const override;
	
private:
	CBaseEntity *m_Actor; // +0x04
};
