/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_mvm_deploy_bomb.h
 * used in MvM: TODO
 */


// sizeof: 0x4c
class CTFBotMvMDeployBomb : public Action<CTFBot>
{
public:
	CTFBotMvMDeployBomb();
	virtual ~CTFBotMvMDeployBomb();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnContact(CTFBot *actor, CBaseEntity *ent, CGameTrace *trace) override;
	
	virtual QueryResponse ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
	
private:
	CountdownTimer m_ctDelay; // +0x34
	// 40 Vector
};
