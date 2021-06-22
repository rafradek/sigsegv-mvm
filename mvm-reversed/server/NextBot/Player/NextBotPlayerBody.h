/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/Player/NextBotPlayerBody.h
 */


class PlayerBody : public IBody
{
public:
	PlayerBody(INextBot *nextbot);
	virtual ~PlayerBody();
	
	virtual void Reset() override;
	virtual void Upkeep() override;
	
	virtual bool SetPosition(const Vector& pos) override;
	virtual Vector& GetEyePosition() const override;
	virtual Vector& GetViewVector() const override;
	virtual void AimHeadTowards(const Vector& vec, IBody::LookAtPriorityType priority, float duration, INextBotReply *reply, const char *reason) override;
	virtual void AimHeadTowards(CBaseEntity *ent, IBody::LookAtPriorityType priority, float duration, INextBotReply *reply, const char *reason) override;
	virtual bool IsHeadAimingOnTarget() const override;
	virtual bool IsHeadSteady() const override;
	virtual float GetHeadSteadyDuration() const override;
	virtual void ClearPendingAimReply() override;
	virtual float GetMaxHeadAngularVelocity() const override;
	virtual bool StartActivity(Activity a1, unsigned int i1) override;
	virtual Activity GetActivity() const override;
	virtual bool IsActivity(Activity a1) const override;
	virtual bool HasActivityType(unsigned int i1) const override;
	virtual void SetDesiredPosture(PostureType posture) override;
	virtual PostureType GetDesiredPosture() const override;
	virtual bool IsDesiredPosture(PostureType posture) const override;
	virtual bool IsInDesiredPosture() const override;
	virtual PostureType GetActualPosture() const override;
	virtual bool IsActualPosture(PostureType posture) const override;
	virtual bool IsPostureMobile() const override;
	virtual bool IsPostureChanging() const override;
	virtual void SetArousal(ArousalType arousal) override;
	virtual ArousalType GetArousal() const override;
	virtual bool IsArousal(ArousalType arousal) const override;
	virtual float GetHullWidth() const override;
	virtual float GetHullHeight() const override;
	virtual float GetStandHullHeight() const override;
	virtual float GetCrouchHullHeight() const override;
	virtual Vector& GetHullMins() const override;
	virtual Vector& GetHullMaxs() const override;
	virtual unsigned int GetSolidMask() const override;
	
	virtual CBaseEntity *GetEntity();
	
private:
	CBasePlayer *m_Player;             // +0x14
	PosureType m_iPosture;             // +0x18
	ArousalType m_iArousal;            // +0x1c
	Vector m_vecEyePosition;           // +0x20
	Vector m_vecEyeVectors;            // +0x2c
	Vector m_vecHullMins;              // +0x38
	Vector m_vecHullMaxs;              // +0x44
	Vector m_vecAimTarget;             // +0x50
	CHandle<CBaseEntity> m_hAimTarget; // +0x5c
	Vector m_vecTargetVelocity;        // +0x60
	CountdownTimer m_ctAimTracking;    // +0x6c
	PriorityType m_iAimPriority;       // +0x78
	CountdownTimer m_ctAimDuration;    // +0x7c
	IntervalTimer m_itAimStart;        // +0x88
	INextBotReply *m_AimReply;         // +0x8c
	bool m_bHeadOnTarget;              // +0x90
	bool m_bSightedIn;                 // +0x91
	IntervalTimer m_itHeadSteady;      // +0x94
	QAngle m_angLastEyeAngles;         // +0x98
	// 0xa4-af: an unused Vector/QAngle perhaps
	CountdownTimer m_ctResettle;       // +0xb0
	Vector m_vecLastEyeVectors;        // +0xbc
};
