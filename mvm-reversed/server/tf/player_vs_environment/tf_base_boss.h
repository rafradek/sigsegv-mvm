/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/player_vs_environment/tf_base_boss.h
 */


class CTFBaseBoss : public NextBotCombatCharacter
{
public:
	CTFBaseBoss();
	virtual ~CTFBaseBoss();
	
	virtual ??? UpdateTransmitState(???) ??? override;
	virtual void Spawn() override;
	virtual void Precache() override;
	virtual ??? OnTakeDamage(???) ??? override;
	virtual ??? Event_Killed(???) ??? override;
	virtual ??? Touch(???) ??? override;
	virtual ??? UpdateOnRemove(???) ??? override;
	virtual ??? OnTakeDamage_Alive(???) ??? override;
	
	virtual bool IsRemovedOnReset() const override;
	virtual ILocomotion *GetLocomotionInterface() const override;
	
	virtual void UpdateCollisionBounds();
	virtual int GetCurrencyValue();
	virtual void ModifyDamage(CTakeDamageInfo *info) const;
	
	void BossThink();
	void ResolvePlayerCollision(CTFPlayer *player);
	
private:
	COutputEvent m_outputOnHealthBelow90Percent; // +0x8b8
	COutputEvent m_outputOnHealthBelow80Percent; // +0x8d0
	COutputEvent m_outputOnHealthBelow70Percent; // +0x8e8
	COutputEvent m_outputOnHealthBelow60Percent; // +0x900
	COutputEvent m_outputOnHealthBelow50Percent; // +0x918
	COutputEvent m_outputOnHealthBelow40Percent; // +0x930
	COutputEvent m_outputOnHealthBelow30Percent; // +0x948
	COutputEvent m_outputOnHealthBelow20Percent; // +0x960
	COutputEvent m_outputOnHealthBelow10Percent; // +0x978
	COutputEvent m_outputOnKilled;               // +0x990
	int m_initialHealth;          // +0x9a8
	float m_lastHealthPercentage; // +0x9ac
	string_t m_modelString;       // +0x9b0
	float m_speed;                // +0x9b4
	int m_startDisabled;          // +0x9b8
	bool m_bEnabled;              // +0x9bc
	// 9c0 
	// 9c4 
	// 9c8 
	// 9cc CTFBaseBossLocomotion *
};


class CTFBaseBossLocomotion : public NextBotGroundLocomotion
{
public:
	CTFBaseBossLocomotion();
	virtual ~CTFBaseBossLocomotion();
	
	virtual void FaceTowards(const Vector& v1) override;
	virtual float GetStepHeight() const override;
	virtual float GetMaxJumpHeight() const override;
	virtual float GetRunSpeed() const override;
	
private:
	// TODO
};
