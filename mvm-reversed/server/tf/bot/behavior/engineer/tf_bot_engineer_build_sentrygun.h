/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/engineer/tf_bot_engineer_build_sentrygun.h
 * used in MvM: TODO
 */


// sizeof: 0x486c
class CTFBotEngineerBuildSentryGun : public Action<CTFBot>
{
public:
	CTFBotEngineerBuildSentryGun();
	CTFBotEngineerBuildSentryGun(CTFBotHintSentrygun *hint);
	virtual ~CTFBotEngineerBuildSentryGun();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
private:
	// 0034 CountdownTimer
	// 0040 CountdownTimer
	// 004c CountdownTimer
	CountdownTimer m_ctRecomputePath; // +0x0058
	// 0064 CountdownTimer
	// 0070 int
	PathFollower m_PathFollower;      // +0x0074
	CTFBotHintSentrygun *m_pHint;     // +0x4848
	Vector m_vecTarget;               // +0x484c
	// 4858 int
	// 485c bool
	// 4860 
	// 4864 
	// 4868 
};
