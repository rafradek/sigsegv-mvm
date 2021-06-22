/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/engineer/mvm_engineer/tf_bot_mvm_engineer_build_teleporter.h
 * used in MvM: TODO
 */


// sizeof: 0x4824
class CTFBotMvMEngineerBuildTeleportExit : public Action<CTFBot>
{
public:
	CTFBotMvMEngineerBuildTeleportExit(CTFBotHintTeleportExit *hint);
	virtual ~CTFBotMvMEngineerBuildTeleportExit();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
private:
	CHandle<CTFBotHintTeleportExit> m_hintEntity; // +0x0034
	CountdownTimer m_ctPushAway;                  // +0x0038
	CountdownTimer m_ctRecomputePath;             // +0x0044
	PathFollower m_PathFollower;                  // +0x0050
};
