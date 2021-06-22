/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_taunt.h
 * used in MvM: TODO
 */


// sizeof: 0x50
class CTFBotTaunt : public Action<CTFBot>
{
public:
	CTFBotTaunt();
	virtual ~CTFBotTaunt();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
private:
	CountdownTimer m_ctWait;  // +0x34
	CountdownTimer m_ctTaunt; // +0x40
	bool m_bTaunting;         // +0x4c
};
