/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_melee_attack.h
 * used in MvM: TODO
 */


// sizeof: 0x4838
class CTFBotMeleeAttack : public Action<CTFBot>
{
public:
	CTFBotMeleeAttack(float abandon_range = -1.0f);
	virtual ~CTFBotMeleeAttack();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
private:
	float m_flAbandonRange; // +0x0034
	ChasePath m_ChasePath;  // +0x0038
};
