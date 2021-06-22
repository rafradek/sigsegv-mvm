/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/medic/tf_bot_medic_retreat.h
 * used in MvM: TODO
 */


// sizeof: 0x4814
class CTFBotMedicRetreat : public Action<CTFBot>
{
public:
	CTFBotMedicRetreat();
	virtual ~CTFBotMedicRetreat();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnMoveToSuccess(CTFBot *actor, const Path *path) override;
	virtual EventDesiredResult<CTFBot> OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail) override;
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	
	virtual QueryResponse ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
	
private:
	PathFollower m_PathFollower;        // +0x0034
	CountdownTimer m_ctLookForPatients; // +0x4808
};


class CUsefulHealTargetFilter : public INextBotEntityFilter
{
public:
	CUsefulHealTargetFilter(int teamnum);
	
	virtual bool IsAllowed(CBaseEntity *ent) const override;
	
private:
	int m_iTeamNum; // +0x04
};
