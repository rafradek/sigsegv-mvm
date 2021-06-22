/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/engineer/tf_bot_engineer_build_teleport_exit.h
 * used in MvM: TODO
 */


// sizeof: 0x484c
class CTFBotEngineerBuildTeleportExit : public Action<CTFBot>
{
public:
	CTFBotEngineerBuildTeleportExit();
	CTFBotEngineerBuildTeleportExit(const Vector& v1, float f1);
	virtual ~CTFBotEngineerBuildTeleportExit();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	
private:
	PathFollower m_PathFollower; // +0x0034
	// 4808 bool
	// 480c Vector
	// 4818 float
	// 481c CountdownTimer
	// 4828 CountdownTimer
	// 4834 CountdownTimer
	// 4840 CountdownTimer
};
