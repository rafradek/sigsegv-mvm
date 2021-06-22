/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/nav_entities/tf_bot_nav_ent_move_to.h
 * used in MvM: TODO
 */


// sizeof: 0x4834
class CTFBotNavEntMoveTo : public Action<CTFBot>
{
public:
	CTFBotNavEntMoveTo(const CFuncNavPrerequisite *prereq);
	virtual ~CTFBotNavEntMoveTo();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
private:
	CHandle<CFuncNavPrerequisite> m_hPrereq; // +0x0034
	Vector m_vecGoalPos;                     // +0x0038
	CNavArea *m_GoalArea;                    // +0x0044
	CountdownTimer m_ctWaitDuration;         // +0x0048
	PathFollower m_PathFollower;             // +0x0054
	CountdownTimer m_ctRecomputePath;        // +0x4828
};
