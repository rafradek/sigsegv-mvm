/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/medic/tf_bot_medic_heal.h
 * used in MvM: TODO
 */


// sizeof: 0x9058
class CTFBotMedicHeal : public Action<CTFBot>
{
public:
	CTFBotMedicHeal();
	virtual ~CTFBotMedicHeal();
	
	virtual const char *GetName() const override;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
	virtual EventDesiredResult<CTFBot> OnMoveToSuccess(CTFBot *actor, const Path *path) override;
	virtual EventDesiredResult<CTFBot> OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail) override;
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	virtual EventDesiredResult<CTFBot> OnActorEmoted(CTFBot *actor, CBaseCombatCharacter *who, int concept) override;
	
	virtual QueryResponse ShouldHurry(const INextBot *nextbot) const override;
	virtual QueryResponse ShouldRetreat(const INextBot *nextbot) const override;
	virtual QueryResponse ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override;
	
private:
	void ComputeFollowPosition(CTFBot *actor);
	bool IsGoodUberTarget(CTFPlayer *player) const;
	bool IsReadyToDeployUber(const CWeaponMedigun *medigun) const;
	bool IsStable(CTFPlayer *player) const;
	bool IsVisibleToEnemy(CTFBot *actor, const Vector& v1) const;
	CTFPlayer *SelectPatient(CTFBot *actor, CTFPlayer *old_patient);
	
	ChasePath m_ChasePath;         // +0x0034
	// 0x4834 CountdownTimer
	// 0x4840 CountdownTimer
	CHandle<CTFPlayer> m_hPatient; // +0x484c
	Vector m_vecPatientPosition;   // +0x4850
	// 0x485c CountdownTimer
	// 0x4868 dword
	// 0x486c CountdownTimer
	PathFollower m_PathFollower;   // +0x4878
	Vector m_vecFollowPosition;    // +0x904c
};


class CKnownCollector : public IVision::IForEachKnownEntity
{
public:
	virtual bool Inspect(const CKnownEntity& known) override;
	
	CUtlVector<const CKnownEntity *> m_KnownEntities; // +0x04
};


class CFindMostInjuredNeighbor : public IVision::IForEachKnownEntity
{
public:
	CFindMostInjuredNeighbor(CTFBot *medic, float max_range, bool non_buffed) :
		m_pMedic(medic),
		m_flRangeLimit(max_range),
		m_bUseNonBuffedMaxHealth(non_buffed)
	{
		this->m_pMostInjured = nullptr;
		this->m_flHealthRatio = 1.0f;
		this->m_bIsOnFire = false;
	}
	
	virtual bool Inspect(const CKnownEntity& known) override;
	
	CTFBot *m_pMedic;              // +0x04
	CTFPlayer *m_pMostInjured;     // +0x08
	float m_flHealthRatio;         // +0x0c
	bool m_bIsOnFire;              // +0x10
	float m_flRangeLimit;          // +0x14
	bool m_bUseNonBuffedMaxHealth; // +0x18
};


class CSelectPrimaryPatient : public IVision::IForEachKnownEntity
{
public:
	// TODO: ctor!
	
	virtual bool Inspect(const CKnownEntity& known) override;
	
	// SelectPreferred(CTFPlayer *player1, CTFPlayer *player2)
	
	CTFBot *m_pMedic;      // +0x04
	// 08 
	CTFPlayer *m_pPatient; // +0x0c
};
