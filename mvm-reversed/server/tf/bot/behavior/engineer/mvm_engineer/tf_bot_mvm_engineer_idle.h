/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/engineer/mvm_engineer/tf_bot_mvm_engineer_idle.h
 * used in MvM: TODO
 */


// sizeof: 0x4868
class CTFBotMvMEngineerIdle : public Action<CTFBot>
{
public:
	CTFBotMvMEngineerIdle();
	virtual ~CTFBotMvMEngineerIdle();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual QueryResponse ShouldHurry(const INextBot *nextbot) const override;
	virtual QueryResponse ShouldRetreat(const INextBot *nextbot) const override;
	virtual QueryResponse ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
	
private:
	bool ShouldAdvanceNestSpot(CTFBot *actor);
	void TakeOverStaleNest(CBaseTFBotHintEntity *hint, CTFBot *actor);
	void TryToDetonateStaleNest();
	
	PathFollower m_PathFollower;                 // +0x0034
	CountdownTimer m_ctRecomputePath;            // +0x4808
	CountdownTImer m_ctSentrySafe;               // +0x4814
	CountdownTimer m_ctSentryCooldown;           // +0x4820
	CountdownTimer m_ctTeleCooldown;             // +0x482c
	CountdownTimer m_ctFindNestHint;             // +0x4838
	CountdownTimer m_ctAdvanceNestSpot;          // +0x4844
	int m_nTeleportAttempts;                     // +0x4850
	bool m_bTeleportedToHint;                    // +0x4854
	CHandle<CBaseTFBotHintEntity> m_hHintTele;   // +0x4858
	CHandle<CBaseTFBotHintEntity> m_hHintSentry; // +0x485c
	CHandle<CBaseTFBotHintEntity> m_hHintNest;   // +0x4860
	bool m_bTriedToDetonateStaleNest;            // +0x4864
};


class CTFBotMvMEngineerHintFinder
{
public:
	static bool FindHint(bool box_check, bool out_of_range_ok, CHandle<CTFBotHintEngineerNest> *the_hint);
};


struct BombInfo_t
{
	Vector closest_pos;
	float hatch_dist_fwd;
	float hatch_dist_back;
};


CTFBotHintEngineerNest *SelectOutOfRangeNest(const CUtlVector<CTFBotHintEngineerNest *>& nests);
bool GetBombInfo(BombInfo_t *info);
