/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/NextBotVisionInterface.h
 */


class IVision : public INextBotComponent
{
public:
	class IForEachKnownEntity
	{
	public:
		virtual bool Inspect(const CKnownEntity& known) = 0;
	};
	
	enum class FieldOfViewCheckType : int
	{
		USE_FOV       = 0,
		DISREGARD_FOV = 1,
	};
	
	IVision(INextBot *nextbot);
	virtual ~IVision();
	
	virtual void Reset() override;
	virtual void Update() override;
	
	virtual bool ForEachKnownEntity(IForEachKnownEntity& functor);
	virtual void CollectKnownEntities(CUtlVector<CKnownEntity> *knowns);
	virtual const CKnownEntity *GetPrimaryKnownThreat(bool only_recently_visible) const;
	virtual float GetTimeSinceVisible(int teamnum) const;
	virtual const CKnownEntity *GetClosestKnown(int teamnum) const;
	virtual int GetKnownCount(int teamnum, bool only_recently_visible, float range) const;
	virtual const CKnownEntity *GetClosestKnown(const INextBotEntityFilter& filter) const;
	virtual const CKnownEntity *GetKnown(const CBaseEntity *ent) const;
	virtual void AddKnownEntity(CBaseEntity *ent);
	virtual void ForgetEntity(CBaseEntity *ent);
	virtual void ForgetAllKnownEntities();
	virtual void CollectPotentiallyVisibleEntities(CUtlVector<CBaseEntity *> *ents);
	virtual float GetMaxVisionRange() const;
	virtual float GetMinRecognizeTime() const;
	virtual bool IsAbleToSee(CBaseEntity *ent, FieldOfViewCheckType ctype, Vector *v1) const;
	virtual bool IsAbleToSee(const Vector& vec, FieldOfViewCheckType ctype) const;
	virtual bool IsIgnored(CBaseEntity *ent) const;
	virtual bool IsVisibleEntityNoticed(CBaseEntity *ent) const;
	virtual bool IsInFieldOfView(const Vector& vec) const;
	virtual bool IsInFieldOfView(CBaseEntity *ent) const;
	virtual float GetDefaultFieldOfView() const;
	virtual float GetFieldOfView() const;
	virtual void SetFieldOfView(float fov);
	virtual bool IsLineOfSightClear(const Vector& v1) const;
	virtual bool IsLineOfSightClearToEntity(const CBaseEntity *ent, Vector *v1) const;
	virtual bool IsLookingAt(const Vector& v1, float cos_half_fov) const;
	virtual bool IsLookingAt(const CBaseCombatCharacter *who, float cos_half_fov) const;
	
protected:
	void UpdateKnownEntities();
	
	CountdownTimer m_Timer1;                  // +0x14
	uint8_t pad[0xb0-0x0c];/*
	float m_flFOV;                            // +0x20
	float m_flCosHalfFOV;                     // +0x24
	CUtlVector<CKnownEntity> m_KnownEntities; // +0x28
	CHandle<CBaseEntity> m_hPrimaryThreat;    // +0x3c
	float m_flLastUpdate;                     // +0x40
	IntervalTimer m_itTeamVisible[32];        // +0x44*/
};
