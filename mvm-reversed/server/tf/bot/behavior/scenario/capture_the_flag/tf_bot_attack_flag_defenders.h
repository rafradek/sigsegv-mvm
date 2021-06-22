/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/scenario/capture_the_flag/tf_bot_attack_flag_defenders.h
 * used in MvM: TODO
 */


// sizeof: 0xd810 (sizeof base: 0x9014)
/* unusual: this is derived from CTFBotAttack, not Action<CTFBot> directly */
class CTFBotAttackFlagDefenders : public CTFBotAttack
{
public:
	CTFBotAttackFlagDefenders(float duration = -1.0f);
	virtual ~CTFBotAttackFlagDefenders();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
private:
	CountdownTimer m_ctActionDuration; // +0x9014
	CountdownTimer m_ctCheckFlag;      // +0x9020
	CHandle<CTFPlayer> m_hTarget;      // +0x902c
	PathFollower m_PathFollower;       // +0x9030
	CountdownTimer m_ctRecomputePath;  // +0xd804
};
