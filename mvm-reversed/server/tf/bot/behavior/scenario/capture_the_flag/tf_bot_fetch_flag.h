/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/scenario/capture_the_flag/tf_bot_fetch_flag.h
 * used in MvM: TODO
 */


// sizeof: 0x4814
class CTFBotFetchFlag : public Action<CTFBot>
{
public:
	CTFBotFetchFlag(bool give_up_when_done);
	virtual ~CTFBotFetchFlag();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual QueryResponse ShouldHurry(const INextBot *nextbot) const override;
	virtual QueryResponse ShouldRetreat(const INextBot *nextbot) const override;
	
private:
	bool m_bGiveUpWhenDone;           // +0x0032
	PathFollower m_PathFollower;      // +0x0034
	CountdownTimer m_ctRecomputePath; // +0x4808
};
