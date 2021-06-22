/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_tactical_monitor.h
 * used in MvM: TODO
 */


// sizeof: 0x7c
class CTFBotTacticalMonitor : public Action<CTFBot>
{
public:
	CTFBotTacticalMonitor();
	virtual ~CTFBotTacticalMonitor();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual Action<CTFBot> *InitialContainedAction(CTFBot *actor) override;
	
	virtual EventDesiredResult<CTFBot> OnOtherKilled(CTFBot *actor, CBaseCombatCharacter *who, const CTakeDamageInfo& info) override;
	virtual EventDesiredResult<CTFBot> OnNavAreaChanged(CTFBot *actor, CNavArea *area1, CNavArea *area2) override;
	virtual EventDesiredResult<CTFBot> OnCommandString(CTFBot *actor, const char *cmd) override;
	
private:
	void AvoidBumpingEnemies(CTFBot *actor);
	CObjectTeleporter *FindNearbyTeleporter(CTFBot *actor);
	void MonitorArmedStickybombs(CTFBot *actor);
	bool ShouldOpportunisticallyTeleport(CTFBot *actor) const;
	
	// 34 CountdownTimer
	// 40 CountdownTimer (related to taunting at humans)
	// 4c CountdownTimer (related to taunting at humans)
	// 58 CountdownTimer (related to taunting at humans)
	// 64 CountdownTimer
	// 70 CountdownTimer
};
