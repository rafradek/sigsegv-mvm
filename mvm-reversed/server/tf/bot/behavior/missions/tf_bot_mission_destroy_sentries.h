/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/missions/tf_bot_mission_destroy_sentries.h
 * used in MvM: TODO
 */


// sizeof: TODO (>=0x38)
class CTFBotMissionDestroySentries : public Action<CTFBot>
{
public:
	CTFBotMissionDestroySentries(CObjectSentrygun *sentry);
	virtual ~CTFBotMissionDestroySentries();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
	
private:
	CObjectSentrygun *SelectSentryTarget(CTFBot *actor);
	
	CHandle<CObjectSentrygun> m_hSentry; // +0x34
};
