/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/nav_entities/tf_bot_nav_ent_wait.h
 * used in MvM: TODO
 */


// sizeof: 0x44
class CTFBotNavEntWait : public Action<CTFBot>
{
public:
	CTFBotNavEntWait(const CFuncNavPrerequisite *prereq);
	virtual ~CTFBotNavEntWait();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
private:
	CHandle<CFuncNavPrerequisite> m_hPrereq; // +0x34
	CountdownTimer m_ctWait;                 // +0x38
};
