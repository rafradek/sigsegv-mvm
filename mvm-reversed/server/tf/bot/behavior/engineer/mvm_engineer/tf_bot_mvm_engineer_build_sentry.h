/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/engineer/mvm_engineer/tf_bot_mvm_engineer_build_sentry.h
 * used in MvM: TODO
 */


// sizeof: 0x4828
class CTFBotMvMEngineerBuildSentryGun : public Action<CTFBot>
{
public:
	CTFBotMvMEngineerBuildSentryGun(CTFBotHintSentrygun *hint);
	virtual ~CTFBotMvMEngineerBuildSentryGun();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
	
private:
	CHandle<CTFBotHintSentrygun> m_hintEntity; // +0x0034
	CHandle<CObjectSentrygun> m_hSentry;       // +0x0038
	CountdownTimer m_ctPushAway;               // +0x003c
	CountdownTimer m_ctRecomputePath;          // +0x0048
	PathFollower m_PathFollower;               // +0x0054
};
