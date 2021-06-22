/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/spy/tf_bot_spy_lurk.h
 * used in MvM: TODO
 */


// sizeof: 0x40
class CTFBotSpyLurk : public Action<CTFBot>
{
public:
	CTFBotSpyLurk();
	virtual ~CTFBotSpyLurk();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual QueryResponse ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
	
private:
	CountdownTimer m_ctPatience; // +0x34
};
