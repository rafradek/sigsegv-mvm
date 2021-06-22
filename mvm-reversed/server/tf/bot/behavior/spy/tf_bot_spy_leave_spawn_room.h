/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/spy/tf_bot_spy_leave_spawn_room.h
 * used in MvM: TODO
 */


// sizeof: 0x44
class CTFBotSpyLeaveSpawnRoom : public Action<CTFBot>
{
public:
	CTFBotSpyLeaveSpawnRoom();
	virtual ~CTFBotSpyLeaveSpawnRoom();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual QueryResponse ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
	
private:
	CountdownTimer m_ctTeleport; // +0x34
	int m_nDistance;             // +0x40
};


bool TeleportNearVictim(CTFBot *spy, CTFPlayer *victim, int i1);
