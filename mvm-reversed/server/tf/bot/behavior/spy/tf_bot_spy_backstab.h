/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/spy/tf_bot_spy_backstab.h
 * used in MvM: TODO
 */


// sizeof: TODO
class CTFBotSpyBackstab : public Action<CTFBot>
{
public:
	CTFBotSpyBackstab();
	virtual ~CTFBotSpyBackstab();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual QueryResponse ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
};
