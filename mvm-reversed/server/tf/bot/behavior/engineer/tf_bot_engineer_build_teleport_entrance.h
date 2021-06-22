/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/engineer/tf_bot_engineer_build_teleport_entrance.h
 * used in MvM: TODO
 */


// sizeof: 0x4808
class CTFBotEngineerBuildTeleportEntrance : public Action<CTFBot>
{
public:
	CTFBotEngineerBuildTeleportEntrance();
	virtual ~CTFBotEngineerBuildTeleportEntrance();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	
private:
	PathFollower m_PathFollower; // +0x0034
};
