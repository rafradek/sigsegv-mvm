/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/training/tf_bot_training.h
 * used in MvM: TODO
 */


// sizeof: 0x4824
class CTFGotoActionPoint : public Action<CTFBot>
{
public:
	CTFGotoActionPoint(/* TODO */);
	virtual ~CTFGotoActionPoint();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
private:
	// TODO
};


// sizeof: 0x4814
class CTFTrainingAttackSentryActionPoint : public Action<CTFBot>
{
public:
	CTFTrainingAttackSentryActionPoint(/* TODO */);
	virtual ~CTFTrainingAttackSentryActionPoint();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
private:
	// TODO
};


// sizeof: 0x34
class CTFDespawn : public Action<CTFBot>
{
public:
	CTFDespawn(/* TODO */);
	virtual ~CTFDespawn();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
};
