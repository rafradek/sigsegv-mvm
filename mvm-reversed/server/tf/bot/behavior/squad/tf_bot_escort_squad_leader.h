/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/squad/tf_bot_escort_squad_leader.h
 * used in MvM: TODO
 */


// sizeof: 0x905c
class CTFBotEscortSquadLeader : public Action<CTFBot>
{
public:
	CTFBotEscortSquadLeader(Action<CTFBot *> *done_action);
	virtual ~CTFBotEscortSquadLeader();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
	
private:
	Action<CTFBot *> *m_DoneAction;   // +0x0034
	CTFBotMeleeAttack m_MeleeAttack;  // +0x0038
	PathFollower m_PathFollower;      // +0x4870
	CountdownTimer m_ctRecomputePath; // +0x9044
	Vector m_vecLeaderGoalDirection;  // +0x9050
};


// sizeof: 0x40
class CTFBotWaitForOutOfPositionSquadMember : public Action<CTFBot>
{
public:
	CTFBotWaitForOutOfPositionSquadMember();
	virtual ~CTFBotWaitForOutOfPositionSquadMember();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
private:
	CountdownTimer m_ctTimeout; // +0x34
};
