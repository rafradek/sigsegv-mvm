/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/spy/tf_bot_spy_hide.h
 * used in MvM: TODO
 */


// sizeof: 0x483c
class CTFBotSpyHide : public Action<CTFBot>
{
public:
	CTFBotSpyHide(CTFPlayer *victim);
	virtual ~CTFBotSpyHide();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnMoveToSuccess(CTFBot *actor, const Path *path) override;
	virtual EventDesiredResult<CTFBot> OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail) override;
	
	virtual QueryResponse ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
	
private:
	bool FindHidingSpot(CTFBot *actor);
	
	CHandle<CTFPlayer> m_hVictim;      // +0x0034
	HidingSpot *m_HidingSpot;          // +0x0038
	CountdownTimer m_ctFindHidingSpot; // +0x003c
	PathFollower m_PathFollower;       // +0x0048
	CountdownTimer m_ctRecomputePath;  // +0x481c
	bool m_bAtHidingSpot;              // +0x4828
	float m_flEnemyIncursionDistance;  // +0x482c
	CountdownTimer m_ctTeaseVictim;    // +0x4830
};


struct IncursionEntry_t
{
	int teamnum;      // +0x00
	CTFNavArea *area; // +0x04
};


class SpyHideIncursionDistanceLess
{
public:
	bool Less(const IncursionEntry_t& lhs, const IncursionEntry_t& rhs, void *ctx);
};
