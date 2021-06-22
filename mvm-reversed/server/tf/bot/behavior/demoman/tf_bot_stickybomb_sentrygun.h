/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/demoman/tf_bot_stickybomb_sentrygun.h
 * used in MvM: TODO
 */


// sizeof: 0x7c
class CTFBotStickybombSentrygun : public Action<CTFBot>
{
public:
	CTFBotStickybombSentrygun(CObjectSentrygun *sentry);
	CTFBotStickybombSentrygun(CObjectSentrygun *sentry, float x, float y, float z);
	virtual ~CTFBotStickybombSentrygun();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> OnSuspend(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnInjured(CTFBot *actor, const CTakeDamageInfo& info) override;
	
	virtual QueryResponse ShouldHurry(const INextBot *nextbot) const override;
	virtual QueryResponse ShouldRetreat(const INextBot *nextbot) const override;
	virtual QueryResponse ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
	
private:
	bool IsAimOnTarget(CTFBot *actor, float pitch, float yaw, float speed);
	
	// 34 Vector/QAngle
	bool m_bOpportunistic;               // +0x40
	bool m_bReload;                      // +0x41
	CHandle<CObjectSentrygun> m_hSentry; // +0x44
	// 48 bool
	CountdownTimer m_ctAimTimeout;       // +0x4c
	// 58 bool
	Vector m_vecAimTarget;               // +0x5c
	// 68 Vector/QAngle
	float m_flChargeLevel;               // +0x74
	// 78 dword
};
