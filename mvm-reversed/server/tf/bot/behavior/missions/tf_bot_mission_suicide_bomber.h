/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/missions/tf_bot_mission_suicide_bomber.h
 * used in MvM: TODO
 */


// sizeof: 0x4850
class CTFBotMissionSuicideBomber : public Action<CTFBot>
{
public:
	CTFBotMissionSuicideBomber();
	virtual ~CTFBotMissionSuicideBomber();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	virtual EventDesiredResult<CTFBot> OnKilled(CTFBot *actor, const CTakeDamageInfo& info) override;
	
	virtual QueryResponse ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
	
private:
	void StartDetonate(CTFBot *actor, bool reached_goal, bool lost_all_health);
	void Detonate(CTFBot *actor);
	
	CHandle<CBaseEntity> m_hTarget;   // +0x0034
	Vector m_vecTargetPos;            // +0x0038
	PathFollower m_PathFollower;      // +0x0044
	CountdownTimer m_ctRecomputePath; // +0x4818, fires every 0.5-1.0 seconds to recompute path
	CountdownTimer m_ctPlaySound;     // +0x4824, fires every 4 seconds to play sound "MVM.SentryBusterIntro"
	CountdownTimer m_ctDetonation;    // +0x4830, started when detonation sequence begins (duration: 2.0 seconds)
	bool m_bDetonating;               // +0x483c
	bool m_bDetReachedGoal;           // +0x483d
	bool m_bDetLostAllHealth;         // +0x483e
	int m_nConsecutivePathFailures;   // +0x4840
	Vector m_vecDetonatePos;          // +0x4844
};
