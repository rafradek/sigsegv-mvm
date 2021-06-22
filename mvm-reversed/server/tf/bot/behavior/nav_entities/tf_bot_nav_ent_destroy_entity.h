/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/nav_entities/tf_bot_nav_ent_destroy_entity.h
 * used in MvM: TODO
 */


// sizeof: TODO (>=0x4819)
class CTFBotNavEntDestroyEntity : public Action<CTFBot>
{
public:
	CTFBotNavEntDestroyEntity(const CFuncNavPrerequisite *prereq);
	virtual ~CTFBotNavEntDestroyEntity();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
	
private:
	void DetonateStickiesWhenSet(CTFBot *actor, CTFPipebombLauncher *launcher) const;
	
	CHandle<CFuncNavPrerequisite> m_hPrereq; // +0x0034
	PathFollower m_PathFollower;             // +0x0038
	CountdownTimer m_ctRecomputePath;        // +0x480c
	// 4818 bool
};
