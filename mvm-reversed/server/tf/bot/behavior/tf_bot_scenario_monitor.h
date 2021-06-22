/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_scenario_monitor.h
 * used in MvM: TODO
 */


// sizeof: 0x4c
class CTFBotScenarioMonitor : public Action<CTFBot>
{
public:
	CTFBotScenarioMonitor();
	virtual ~CTFBotScenarioMonitor();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual Action<CTFBot> *InitialContainedAction(CTFBot *actor) override;
	
private:
	virtual Action<CTFBot> *DesiredScenarioAndClassAction(CTFBot *actor);
	
	CountdownTimer m_ctFetchFlagInitial; // +0x34
	CountdownTimer m_ctFetchFlag;        // +0x40
};
